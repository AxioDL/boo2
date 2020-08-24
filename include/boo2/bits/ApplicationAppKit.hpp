#pragma once

#include "ApplicationCommon.hpp"

#include <AppKit/AppKit.h>
#include <pwd.h>

@interface Application : NSApplication <NSApplicationDelegate>
@end

#ifdef BOO2_IMPLEMENTATION
@implementation Application
- (id)init {
  self = [super init];
  self.activationPolicy = NSApplicationActivationPolicyRegular;
  self.delegate = self;

  NSMenu* Menu = [NSMenu new];
  NSMenuItem* MenuItem = [NSMenuItem new];
  [Menu addItem:MenuItem];
  NSMenu* AppMenu = [NSMenu new];
  NSString* AppName = [[NSProcessInfo processInfo] processName];
  NSString* QuitTitle = [@"Quit " stringByAppendingString:AppName];
  NSMenuItem* QuitMenuItem =
      [[NSMenuItem alloc] initWithTitle:QuitTitle
                                 action:@selector(terminate:)
                          keyEquivalent:@"q"];
  [AppMenu addItem:QuitMenuItem];
  [MenuItem setSubmenu:AppMenu];
  self.mainMenu = Menu;

  return self;
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
  return YES;
}
- (void)terminate:(id)sender {
  /* Use -stop in place of -terminate so -run returns */
  [self stop:sender];
}
@end
#endif

@class CocoaWindow;

namespace boo2::detail {
class IAppKitEventHandler {
public:
#define APPKIT_EVENT(name)                                                     \
  virtual void name(CocoaWindow* window, NSEvent* event) noexcept {}
#include "AppKitEvents.def"
};
} // namespace boo2::detail

@interface MetalView : NSView
- (id)initWithFrame:(NSRect)frameRect
       eventHandler:(boo2::detail::IAppKitEventHandler*)eh;
- (CALayer*)makeBackingLayer;
@end

#ifdef BOO2_IMPLEMENTATION
@implementation MetalView {
  boo2::detail::IAppKitEventHandler* m_eventHandler;
}
- (id)initWithFrame:(NSRect)frameRect
       eventHandler:(boo2::detail::IAppKitEventHandler*)eh {
  self = [super initWithFrame:frameRect];
  self.wantsLayer = YES;
  m_eventHandler = eh;
  return self;
}
- (CALayer*)makeBackingLayer {
  CAMetalLayer* Layer = [CAMetalLayer layer];
  Layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  Layer.opaque = NO;
  Layer.backgroundColor = CGColorGetConstantColor(kCGColorClear);
  return Layer;
}
- (BOOL)layer:(CALayer*)layer
    shouldInheritContentsScale:(CGFloat)newScale
                    fromWindow:(NSWindow*)window {
  return YES;
}
- (BOOL)acceptsFirstResponder {
  return YES;
}
#define APPKIT_EVENT(name)                                                     \
  -(void)name : (NSEvent*)event {                                              \
    m_eventHandler->name((CocoaWindow*)self.window, event);                    \
  }
#include "AppKitEvents.def"
@end
#endif

@interface CocoaWindow : NSWindow {
@public
  boo2::KeyModifier lastMods;
}
- (id)initWithContentRect:(NSRect)rect
             eventHandler:(boo2::detail::IAppKitEventHandler*)eh;
@end

#ifdef BOO2_IMPLEMENTATION
@implementation CocoaWindow
- (id)initWithContentRect:(NSRect)rect
             eventHandler:(boo2::detail::IAppKitEventHandler*)eh {
  self = [super
      initWithContentRect:rect
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskMiniaturizable |
                          NSWindowStyleMaskResizable /*|
                        NSWindowStyleMaskFullSizeContentView*/
                  backing:NSBackingStoreBuffered
                    defer:NO];
  self.contentView = [[MetalView alloc] initWithFrame:rect eventHandler:eh];
  self.releasedWhenClosed = NO;
  self.opaque = NO;
  self.backgroundColor = [NSColor clearColor];
  self.hasShadow = NO;
  lastMods = boo2::KeyModifier::None;

  return self;
}
- (BOOL)acceptsFirstResponder {
  return YES;
}
- (BOOL)acceptsMouseMovedEvents {
  return YES;
}
- (NSWindowCollectionBehavior)collectionBehavior {
  return NSWindowCollectionBehaviorFullScreenPrimary;
}
@end
#endif

namespace boo2 {

template <class Application> class WindowAppKit {
  friend Application;
  static constexpr int32_t MinDim = 256;
  static constexpr int32_t MaxDim = 16384;
  hsh::owner<hsh::surface> m_hshSurface;
  Application* m_parent = nullptr;
  CocoaWindow* m_window = nullptr;
  std::unique_ptr<WindowDecorations> m_decorations;

  explicit WindowAppKit(Application* parent, SystemStringView title, int x,
                        int y, int w, int h) noexcept
      : m_parent(parent), m_decorations(new WindowDecorations()) {
    m_window = [[CocoaWindow alloc] initWithContentRect:NSMakeRect(x, y, w, h)
                                           eventHandler:parent];
    if (!m_window) {
      Application::Log.report(logvisor::Error,
                              FMT_STRING("Unable to create window"));
      return;
    }
    m_window.contentMinSize = NSMakeSize(MinDim, MinDim);
    m_window.contentMaxSize = NSMakeSize(MaxDim, MaxDim);
  }

  bool createSurface(
      std::function<void(const hsh::extent2d&)>&& resizeLambda) noexcept {
    m_hshSurface = hsh::create_surface(
        getCAMetalLayer(),
        [dec = m_decorations.get(), handleResize = std::move(resizeLambda)](
            const hsh::extent2d& extent, const hsh::extent2d& contentExtent) {
          dec->update(extent);
          handleResize(contentExtent);
        },
        {}, {}, WindowDecorations::MarginL, WindowDecorations::MarginR,
        WindowDecorations::MarginT, WindowDecorations::MarginB);
    m_hshSurface.attach_decoration_lambda(
        [dec = m_decorations.get()]() { dec->draw(); });
    return m_hshSurface.operator bool();
  }

  CAMetalLayer* getCAMetalLayer() const noexcept {
    return (CAMetalLayer*)m_window.contentView.layer;
  }

public:
  using ID = CocoaWindow*;
  operator CocoaWindow*() const noexcept { return m_window; }
  ID getID() const noexcept { return m_window; }
  bool operator==(ID window) const noexcept { return m_window == window; }
  bool operator!=(ID window) const noexcept { return m_window != window; }

  WindowAppKit() noexcept = default;

  ~WindowAppKit() noexcept { [m_window close]; }

  WindowAppKit(WindowAppKit&& other) noexcept
      : m_hshSurface(std::move(other.m_hshSurface)), m_parent(other.m_parent),
        m_window(other.m_window),
        m_decorations(std::move(other.m_decorations)) {
    other.m_parent = nullptr;
    other.m_window = nullptr;
  }

  WindowAppKit& operator=(WindowAppKit&& other) noexcept {
    std::swap(m_hshSurface, other.m_hshSurface);
    std::swap(m_parent, other.m_parent);
    std::swap(m_window, other.m_window);
    std::swap(m_decorations, other.m_decorations);
    return *this;
  }

  WindowAppKit(const WindowAppKit&) = delete;
  WindowAppKit& operator=(const WindowAppKit&) = delete;

  bool acquireNextImage() noexcept { return m_hshSurface.acquire_next_image(); }
  hsh::surface getSurface() const noexcept { return m_hshSurface.get(); }
  operator hsh::surface() const noexcept { return getSurface(); }

  void setTitle(SystemStringView title) noexcept {
    m_window.title = @(title.data());
  }
};

class MacOSPaths {
  SystemString m_config;
  SystemString m_data;
  SystemString m_cache;

  static const char* getHome() noexcept {
    const char* homeDir;
    if ((homeDir = std::getenv("HOME")) && homeDir[0] == '/')
      return homeDir;

    struct passwd* pwd;
    if (!(pwd = getpwuid(getuid())) || !pwd->pw_dir || pwd->pw_dir[0] != '/')
      return nullptr;

    return pwd->pw_dir;
  }

  static SystemString
  getApplicationSupportPath(SystemStringView appName,
                            const SystemChar* subdir) noexcept {
    if (const char* home = getHome()) {
      mkdir(fmt::format(FMT_STRING("{}/Library/Application Support/{}"), home,
                        appName)
                .c_str(),
            0755);
      SystemString ret =
          fmt::format(FMT_STRING("{}/Library/Application Support/{}/{}"), home,
                      appName, subdir);
      mkdir(ret.c_str(), 0755);
      return ret;
    }

    return {};
  }

public:
  explicit MacOSPaths(SystemStringView appName) noexcept {
    m_config = getApplicationSupportPath(appName, "config");
    m_data = getApplicationSupportPath(appName, "data");
    m_cache = getApplicationSupportPath(appName, "cache");
  }

  SystemString getConfigPath(SystemStringView subPath) const noexcept {
    return fmt::format(FMT_STRING("{}/{}"), m_config, subPath);
  }

  SystemString getDataPath(SystemStringView subPath) const noexcept {
    return fmt::format(FMT_STRING("{}/{}"), m_data, subPath);
  }

  SystemString getCachePath(SystemStringView subPath) const noexcept {
    return fmt::format(FMT_STRING("{}/{}"), m_cache, subPath);
  }
};

struct MacOSPipelineCacheFileManager {
  const MacOSPaths& m_paths;

  explicit MacOSPipelineCacheFileManager(MacOSPaths& paths) noexcept
      : m_paths(paths) {}

  NSURL* GetFilename() noexcept {
    SystemString path = m_paths.getCachePath("pipeline-cache.bin");
    return [NSURL fileURLWithPath:@(path.c_str())];
  }

  template <typename Func> void ReadPipelineCache(Func F) noexcept {
    F(GetFilename());
  }

  template <typename Func> void WritePipelineCache(Func F) noexcept {
    F(GetFilename());
  }
};

template <template <class, class> class Delegate>
class ApplicationAppKit
    : public ApplicationBase<MetalTraits, MacOSPipelineCacheFileManager>,
      public detail::IAppKitEventHandler {
  Delegate<ApplicationAppKit<Delegate>,
           WindowAppKit<ApplicationAppKit<Delegate>>>
      m_delegate;
  MacOSPaths m_macPaths;
  MacOSPipelineCacheFileManager m_pcfm{m_macPaths};
  friend WindowAppKit<ApplicationAppKit<Delegate>>;
  using Window = WindowAppKit<ApplicationAppKit<Delegate>>;
  static logvisor::Module Log;

  Application* m_app;
  operator Application*() const noexcept { return m_app; }

  bool m_builtWindow = false;

public:
  [[nodiscard]] Window createWindow(SystemStringView title, int x, int y, int w,
                                    int h) noexcept {
    if (m_builtWindow)
      Log.report(logvisor::Fatal,
                 FMT_STRING("boo2 currently only supports one window"));

    w += WindowDecorations::MarginL + WindowDecorations::MarginR;
    h += WindowDecorations::MarginT + WindowDecorations::MarginB;

    Window window(this, title, x, y, w, h);
    if (!window)
      return {};

    if (!this->m_device) {
      this->m_device = MetalDevice(this->m_instance, *this, this->m_delegate,
                                   window.getCAMetalLayer());
      if (!this->m_device) {
        Log.report(logvisor::Error,
                   FMT_STRING("No valid Metal devices accepted"));
        return {};
      }
      m_buildingPipelines = true;
    }

    CocoaWindow* windowObj = window.getID();
    if (!window.createSurface([this, windowObj](const hsh::extent2d& extent) {
          this->m_delegate.onWindowResize(*this, windowObj, extent);
        })) {
      Log.report(
          logvisor::Error,
          FMT_STRING("Metal surface not compatible with existing device"));
      return {};
    }

    [windowObj makeKeyAndOrderFront:nullptr];

    m_builtWindow = true;
    return window;
  }

  void dispatchLatestEvents() noexcept {
    // Keyboard events are the only ones we can safely handle here.
    // Mouse events may trigger window resizes and we do not want them here.
    while (NSEvent* Event = [m_app
               nextEventMatchingMask:NSEventMaskKeyDown | NSEventMaskKeyUp |
                                     NSEventMaskFlagsChanged
                           untilDate:nullptr
                              inMode:NSDefaultRunLoopMode
                             dequeue:YES])
      [m_app sendEvent:Event];
  }

  void quit(int code = 0) noexcept {
    [m_app stop:nullptr];
    m_exitCode = code;
  }

private:
  int m_exitCode = 0;
  bool m_buildingPipelines = false;

  void toggleFullscreen(CocoaWindow* window) noexcept {
    [window toggleFullScreen:nullptr];
  }

  static void getMods(KeyModifier& mods, NSEventModifierFlags flags) noexcept {
    mods |= flags & NSEventModifierFlagCommand ? KeyModifier::Control
                                               : KeyModifier::None;
    mods |= flags & NSEventModifierFlagOption ? KeyModifier::Alt
                                              : KeyModifier::None;
    mods |= flags & NSEventModifierFlagShift ? KeyModifier::Shift
                                             : KeyModifier::None;
  }

  template <typename UTF32Func, typename SpecialFunc>
  static void translateVirtualKey(NSEvent* event, UTF32Func utfFunc,
                                  SpecialFunc specialFunc) noexcept {
    KeyModifier mods = KeyModifier::None;
    getMods(mods, event.modifierFlags);
    switch (event.keyCode) {
#define BOO2_SPECIAL_KEYCODE(name, xkbcode, vkcode, maccode)                   \
  case maccode:                                                                \
    specialFunc(Keycode::name, mods);                                          \
    break;
#include "SpecialKeycodes.def"
    default:
      for (NSUInteger i = 0; i < event.characters.length; ++i)
        utfFunc([event.characters characterAtIndex:i], mods);
      break;
    }
  }

  static MouseButton translateButton(NSEvent* event) noexcept {
    switch (event.buttonNumber) {
    default:
    case 3:
      return MouseButton::Middle;
    case 4:
      return MouseButton::Aux1;
    case 5:
      return MouseButton::Aux2;
    }
  }

  virtual void keyDown(CocoaWindow* w, NSEvent* event) noexcept final {
    translateVirtualKey(
        event,
        [this, w](uint32_t sym, KeyModifier mods) {
          if (sym == '\r' && (mods & KeyModifier::Alt) != KeyModifier::None)
            toggleFullscreen(w);
          m_delegate.onUtf32Pressed(*this, w, sym, mods);
        },
        [this, w](Keycode code, KeyModifier mods) {
          m_delegate.onSpecialKeyPressed(*this, w, code, mods);
        });
  }

  virtual void keyUp(CocoaWindow* w, NSEvent* event) noexcept final {
    translateVirtualKey(
        event,
        [this, w](uint32_t sym, KeyModifier mods) {
          m_delegate.onUtf32Released(*this, w, sym, mods);
        },
        [this, w](Keycode code, KeyModifier mods) {
          m_delegate.onSpecialKeyReleased(*this, w, code, mods);
        });
  }

  virtual void mouseEntered(CocoaWindow* w, NSEvent* event) noexcept final {
    KeyModifier mods = KeyModifier::None;
    getMods(mods, event.modifierFlags);
    m_delegate.onMouseEnter(*this, w,
                            hsh::offset2d(int32_t(event.locationInWindow.x),
                                          int32_t(event.locationInWindow.y)),
                            mods);
  }

  virtual void mouseExited(CocoaWindow* w, NSEvent* event) noexcept final {
    KeyModifier mods = KeyModifier::None;
    getMods(mods, event.modifierFlags);
    m_delegate.onMouseLeave(*this, w,
                            hsh::offset2d(int32_t(event.locationInWindow.x),
                                          int32_t(event.locationInWindow.y)),
                            mods);
  }

  virtual void mouseMoved(CocoaWindow* w, NSEvent* event) noexcept final {
    KeyModifier mods = KeyModifier::None;
    getMods(mods, event.modifierFlags);
    m_delegate.onMouseMove(*this, w,
                           hsh::offset2d(int32_t(event.locationInWindow.x),
                                         int32_t(event.locationInWindow.y)),
                           mods);
  }

  void mouseDown(CocoaWindow* w, NSEvent* event, MouseButton button) noexcept {
    KeyModifier mods = KeyModifier::None;
    getMods(mods, event.modifierFlags);
    m_delegate.onMouseDown(*this, w,
                           hsh::offset2d(int32_t(event.locationInWindow.x),
                                         int32_t(event.locationInWindow.y)),
                           button, mods);
  }

  virtual void mouseDown(CocoaWindow* w, NSEvent* event) noexcept final {
    mouseDown(w, event, MouseButton::Primary);
  }

  virtual void rightMouseDown(CocoaWindow* w,
                              NSEvent* event) noexcept final {
    mouseDown(w, event, MouseButton::Secondary);
  }

  virtual void otherMouseDown(CocoaWindow* w,
                              NSEvent* event) noexcept final {
    mouseDown(w, event, translateButton(event));
  }

  void mouseUp(CocoaWindow* w, NSEvent* event, MouseButton button) noexcept {
    KeyModifier mods = KeyModifier::None;
    getMods(mods, event.modifierFlags);
    m_delegate.onMouseUp(*this, w,
                         hsh::offset2d(int32_t(event.locationInWindow.x),
                                       int32_t(event.locationInWindow.y)),
                         button, mods);
  }

  virtual void mouseUp(CocoaWindow* w, NSEvent* event) noexcept final {
    mouseUp(w, event, MouseButton::Primary);
  }

  virtual void rightMouseUp(CocoaWindow* w, NSEvent* event) noexcept final {
    mouseUp(w, event, MouseButton::Secondary);
  }

  virtual void otherMouseUp(CocoaWindow* w, NSEvent* event) noexcept final {
    mouseUp(w, event, translateButton(event));
  }

  virtual void scrollWheel(CocoaWindow* w, NSEvent* event) noexcept final {
    KeyModifier mods = KeyModifier::None;
    getMods(mods, event.modifierFlags);
    m_delegate.onScroll(
        *this, w, hsh::offset2d(int32_t(event.deltaX), int32_t(event.deltaY)),
        mods);
  }

  bool pumpBuildPipelines() noexcept {
    return this->pumpBuildRHIPipelines(m_pcfm, *this, m_delegate);
  }

  void dispatchIdleFunc() noexcept {
    if (this->m_device) {
      this->m_device.enter_draw_context([this]() {
        if (m_buildingPipelines)
          m_buildingPipelines = this->pumpBuildPipelines();
        else
          this->m_delegate.onAppIdle(*this);
      });
    }
  }

  void idle() noexcept {
    dispatchIdleFunc();
    if (m_app.isRunning)
      dispatch_async_f(dispatch_get_main_queue(), this,
                       dispatch_function_t(_idle));
  }
  static void _idle(ApplicationAppKit* self) noexcept { self->idle(); }

  int run() noexcept {
    if (!this->m_instance)
      return 1;

    this->m_delegate.onAppLaunched(*this);

    dispatch_async_f(dispatch_get_main_queue(), this,
                     dispatch_function_t(_idle));
    [m_app run];

    this->m_delegate.onAppExiting(*this);

    if (this->m_device)
      this->m_device.wait_idle();
    return m_exitCode;
  }

  template <typename... DelegateArgs>
  explicit ApplicationAppKit(int argc, SystemChar** argv,
                             SystemStringView appName,
                             DelegateArgs&&... args) noexcept
      : ApplicationBase(argc, argv, appName), m_macPaths(appName),
        m_app([Application sharedApplication]) {}

public:
  ApplicationAppKit(ApplicationAppKit&&) = delete;
  ApplicationAppKit& operator=(ApplicationAppKit&&) = delete;
  ApplicationAppKit(const ApplicationAppKit&) = delete;
  ApplicationAppKit& operator=(const ApplicationAppKit&) = delete;

  ~ApplicationAppKit() noexcept { WindowDecorations::shutdown(); }

  template <typename... DelegateArgs>
  static int exec(int argc, SystemChar** argv, SystemStringView appName,
                  DelegateArgs&&... args) noexcept {
    ApplicationAppKit app(argc, argv, appName,
                          std::forward<DelegateArgs>(args)...);
    return app.run();
  }
};
template <template <class, class> class Delegate>
logvisor::Module ApplicationAppKit<Delegate>::Log("boo2::ApplicationAppKit");

template <template <class, class> class Delegate> class ApplicationAppKitExec {
public:
  template <typename... DelegateArgs>
  static int exec(int argc, SystemChar** argv, SystemStringView appName,
                  DelegateArgs&&... args) noexcept {
    return ApplicationAppKit<Delegate>::exec(
        argc, argv, appName, std::forward<DelegateArgs>(args)...);
  }
};

} // namespace boo2
