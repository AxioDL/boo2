#pragma once

#include "ApplicationCommon.hpp"

#include <UIKit/UIKit.h>

@class CocoaWindow;

namespace boo2::detail {
class IUIKitEventHandler {
public:
#define UIKIT_EVENT(name, objtype, eventtype)                                  \
  virtual void name(CocoaWindow* window, objtype obj,                          \
                    eventtype event) noexcept {}
#include "UIKitEvents.def"
  virtual void applicationDidFinishLaunching(UIApplication* app) noexcept {}
  virtual void applicationDidBecomeActive(UIApplication* app) noexcept {}
  virtual void applicationWillResignActive(UIApplication* app) noexcept {}
  virtual void applicationWillTerminate(UIApplication* app) noexcept {}
  virtual void sceneWillConnectToSession(UIScene* scene,
                                         UIWindow* window) noexcept {}
  static IUIKitEventHandler* Instance;
};
#ifdef BOO2_IMPLEMENTATION
IUIKitEventHandler* IUIKitEventHandler::Instance = nullptr;
#endif
} // namespace boo2::detail

@interface CocoaWindow : UIWindow
@end

#ifdef BOO2_IMPLEMENTATION
@implementation CocoaWindow
+ (Class)layerClass {
  return [CAMetalLayer class];
}
#define UIKIT_EVENT(name, objtype, eventtype)                                  \
  -(void)name : (objtype)obj withEvent : (eventtype)event {                    \
    boo2::detail::IUIKitEventHandler::Instance->name(self, obj, event);        \
  }
#include "UIKitEvents.def"
@end
#endif

@interface SceneDelegate : NSObject <UIWindowSceneDelegate>
@end

#ifdef BOO2_IMPLEMENTATION
//#if 1
@implementation SceneDelegate
@synthesize window = _window;
- (void)scene:(UIScene*)scene
    willConnectToSession:(UISceneSession*)session
                 options:(UISceneConnectionOptions*)connectionOptions {
  _window = [[CocoaWindow alloc] initWithWindowScene:(UIWindowScene*)scene];
  boo2::detail::IUIKitEventHandler::Instance->sceneWillConnectToSession(
      scene, _window);
}
- (void)windowScene:(UIWindowScene*)windowScene
    didUpdateCoordinateSpace:(id<UICoordinateSpace>)previousCoordinateSpace
        interfaceOrientation:
            (UIInterfaceOrientation)previousInterfaceOrientation
             traitCollection:(UITraitCollection*)previousTraitCollection {
  _window.layer.frame = windowScene.coordinateSpace.bounds;
}
@end
#endif

@interface AppDelegate : NSObject <UIApplicationDelegate>
@end

#ifdef BOO2_IMPLEMENTATION
@implementation AppDelegate
- (UISceneConfiguration*)application:(UIApplication*)application
    configurationForConnectingSceneSession:
        (UISceneSession*)connectingSceneSession
                                   options:(UISceneConnectionOptions*)options {
  UISceneConfiguration* Ret = [UISceneConfiguration
      configurationWithName:@"main"
                sessionRole:UIWindowSceneSessionRoleApplication];
  Ret.sceneClass = [UIWindowScene class];
  Ret.delegateClass = [SceneDelegate class];
  return Ret;
}
- (void)applicationDidFinishLaunching:(UIApplication*)application {
  boo2::detail::IUIKitEventHandler::Instance->applicationDidFinishLaunching(
      application);
}
- (void)applicationDidBecomeActive:(UIApplication*)application {
  boo2::detail::IUIKitEventHandler::Instance->applicationDidBecomeActive(
      application);
}
- (void)applicationWillResignActive:(UIApplication*)application {
  boo2::detail::IUIKitEventHandler::Instance->applicationWillResignActive(
      application);
}
- (void)applicationWillTerminate:(UIApplication*)application {
  boo2::detail::IUIKitEventHandler::Instance->applicationWillTerminate(
      application);
}
@end
#endif

namespace boo2 {

template <class Application> class WindowUIKit {
  friend Application;
  static constexpr int32_t MinDim = 256;
  static constexpr int32_t MaxDim = 16384;
  hsh::owner<hsh::surface> m_hshSurface;
  Application* m_parent = nullptr;
  UIScene* m_scene = nullptr;
  CocoaWindow* m_window = nullptr;

  explicit WindowUIKit(Application* parent, SystemStringView title, int x,
                       int y, int w, int h) noexcept
      : m_parent(parent), m_scene(parent->m_scene),
        m_window((CocoaWindow*)parent->m_window) {
    m_scene.title = @(title.data());
    if (!m_window)
      Application::Log.report(logvisor::Error,
                              FMT_STRING("Unable to create window"));
  }

  bool createSurface(
      std::function<void(const hsh::extent2d&)>&& resizeLambda) noexcept {
    m_hshSurface = hsh::create_surface(
        getCAMetalLayer(),
        [handleResize = std::move(resizeLambda)](
            const hsh::extent2d& extent, const hsh::extent2d& contentExtent) {
          handleResize(contentExtent);
        });
    return m_hshSurface.operator bool();
  }

  CAMetalLayer* getCAMetalLayer() const noexcept {
    return (CAMetalLayer*)m_window.layer;
  }

public:
  using ID = CocoaWindow*;
  operator CocoaWindow*() const noexcept { return m_window; }
  ID getID() const noexcept { return m_window; }
  bool operator==(ID window) const noexcept { return m_window == window; }
  bool operator!=(ID window) const noexcept { return m_window != window; }

  WindowUIKit() noexcept = default;

  WindowUIKit(WindowUIKit&& other) noexcept
      : m_hshSurface(std::move(other.m_hshSurface)), m_parent(other.m_parent),
        m_scene(other.m_scene), m_window(other.m_window) {
    other.m_parent = nullptr;
    other.m_scene = nullptr;
    other.m_window = nullptr;
  }

  WindowUIKit& operator=(WindowUIKit&& other) noexcept {
    std::swap(m_hshSurface, other.m_hshSurface);
    std::swap(m_parent, other.m_parent);
    std::swap(m_scene, other.m_scene);
    std::swap(m_window, other.m_window);
    return *this;
  }

  WindowUIKit(const WindowUIKit&) = delete;
  WindowUIKit& operator=(const WindowUIKit&) = delete;

  bool acquireNextImage() noexcept { return m_hshSurface.acquire_next_image(); }
  hsh::surface getSurface() const noexcept { return m_hshSurface.get(); }
  operator hsh::surface() const noexcept { return getSurface(); }

  void setTitle(SystemStringView title) noexcept { m_scene = @(title.data()); }
};

class IOSPaths {
  SystemString m_config;
  SystemString m_data;
  SystemString m_cache;

  static SystemString
  getApplicationHomePath(const SystemChar* subdir) noexcept {
    if (NSString* home = NSHomeDirectory()) {
      SystemString ret =
          fmt::format(FMT_STRING("{}/{}"), home.UTF8String, subdir);
      mkdir(ret.c_str(), 0755);
      return ret;
    }

    return {};
  }

public:
  IOSPaths() noexcept {
    m_config = getApplicationHomePath("config");
    m_data = getApplicationHomePath("data");
    m_cache = getApplicationHomePath("cache");
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

struct IOSPipelineCacheFileManager {
  const IOSPaths& m_paths;

  explicit IOSPipelineCacheFileManager(IOSPaths& paths) noexcept
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
class ApplicationUIKit
    : public ApplicationBase<MetalTraits, IOSPipelineCacheFileManager>,
      public detail::IUIKitEventHandler {
  Delegate<ApplicationUIKit<Delegate>, WindowUIKit<ApplicationUIKit<Delegate>>>
      m_delegate;
  IOSPaths m_iosPaths;
  IOSPipelineCacheFileManager m_pcfm{m_iosPaths};
  friend WindowUIKit<ApplicationUIKit<Delegate>>;
  using Window = WindowUIKit<ApplicationUIKit<Delegate>>;
  static logvisor::Module Log;

  UIApplication* m_app = nullptr;
  operator UIApplication*() const noexcept { return m_app; }

  UIScene* m_scene = nullptr;
  UIWindow* m_window = nullptr;

  bool m_builtWindow = false;

public:
  [[nodiscard]] Window createWindow(SystemStringView title, int x, int y, int w,
                                    int h) noexcept {
    if (m_builtWindow)
      Log.report(logvisor::Fatal,
                 FMT_STRING("boo2 currently only supports one window"));

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

    [windowObj makeKeyAndVisible];

    m_builtWindow = true;
    return window;
  }

  void dispatchLatestEvents() noexcept {}

  void quit(int code = 0) noexcept {}

private:
  bool m_running = true;
  bool m_buildingPipelines = false;

  static void getMods(KeyModifier& mods, UIKeyModifierFlags flags) noexcept {
    mods |=
        flags & UIKeyModifierCommand ? KeyModifier::Control : KeyModifier::None;
    mods |=
        flags & UIKeyModifierAlternate ? KeyModifier::Alt : KeyModifier::None;
    mods |= flags & UIKeyModifierShift ? KeyModifier::Shift : KeyModifier::None;
  }

  template <typename UTF32Func, typename SpecialFunc>
  static void translateVirtualKey(UIPress* press, UIEvent* event,
                                  UTF32Func utfFunc,
                                  SpecialFunc specialFunc) noexcept {
    KeyModifier mods = KeyModifier::None;
    getMods(mods, event.modifierFlags);
    switch (press.key.keyCode) {
#define BOO2_SPECIAL_KEYCODE(name, xkbcode, vkcode, maccode, ioscode)          \
  case ioscode:                                                                \
    specialFunc(Keycode::name, mods);                                          \
    break;
#include "SpecialKeycodes.def"
    default:
      for (NSUInteger i = 0; i < press.key.characters.length; ++i)
        utfFunc([press.key.characters characterAtIndex:i], mods);
      break;
    }
  }

  virtual void pressesBegan(CocoaWindow* w, NSSet<UIPress*>* presses,
                            UIPressesEvent* event) noexcept final {
    for (UIPress* press in presses) {
      translateVirtualKey(
          press, event,
          [this, w](uint32_t sym, KeyModifier mods) {
            m_delegate.onUtf32Pressed(*this, w, sym, mods);
          },
          [this, w](Keycode code, KeyModifier mods) {
            m_delegate.onSpecialKeyPressed(*this, w, code, mods);
          });
    }
  }

  virtual void pressesEnded(CocoaWindow* w, NSSet<UIPress*>* presses,
                            UIPressesEvent* event) noexcept final {
    for (UIPress* press in presses) {
      translateVirtualKey(
          press, event,
          [this, w](uint32_t sym, KeyModifier mods) {
            m_delegate.onUtf32Released(*this, w, sym, mods);
          },
          [this, w](Keycode code, KeyModifier mods) {
            m_delegate.onSpecialKeyReleased(*this, w, code, mods);
          });
    }
  }

  virtual void pressesCancelled(CocoaWindow* w, NSSet<UIPress*>* presses,
                                UIPressesEvent* event) noexcept final {
    pressesEnded(w, presses, event);
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
    if (m_running)
      dispatch_async_f(dispatch_get_main_queue(), this,
                       dispatch_function_t(_idle));
  }
  static void _idle(ApplicationUIKit* self) noexcept { self->idle(); }

  virtual void
  applicationDidFinishLaunching(UIApplication* app) noexcept final {
    m_app = app;
  }

  virtual void sceneWillConnectToSession(UIScene* scene,
                                         UIWindow* window) noexcept final {
    m_scene = scene;
    m_window = window;
    this->m_delegate.onAppLaunched(*this);
    dispatch_async_f(dispatch_get_main_queue(), this,
                     dispatch_function_t(_idle));
  }

  virtual void applicationWillTerminate(UIApplication* app) noexcept final {
    m_running = false;
    this->m_delegate.onAppExiting(*this);
  }

  int run(int argc, SystemChar** argv) noexcept {
    if (!this->m_instance)
      return 1;

    detail::IUIKitEventHandler::Instance = this;
    return UIApplicationMain(argc, argv, nullptr, @"AppDelegate");
  }

  template <typename... DelegateArgs>
  explicit ApplicationUIKit(int argc, SystemChar** argv,
                            SystemStringView appName,
                            DelegateArgs&&... args) noexcept
      : ApplicationBase(argc, argv, appName) {}

public:
  ApplicationUIKit(ApplicationUIKit&&) = delete;
  ApplicationUIKit& operator=(ApplicationUIKit&&) = delete;
  ApplicationUIKit(const ApplicationUIKit&) = delete;
  ApplicationUIKit& operator=(const ApplicationUIKit&) = delete;

  ~ApplicationUIKit() noexcept { WindowDecorations::shutdown(); }

  template <typename... DelegateArgs>
  static int exec(int argc, SystemChar** argv, SystemStringView appName,
                  DelegateArgs&&... args) noexcept {
    ApplicationUIKit app(argc, argv, appName,
                         std::forward<DelegateArgs>(args)...);
    return app.run(argc, argv);
  }
};
template <template <class, class> class Delegate>
logvisor::Module ApplicationUIKit<Delegate>::Log("boo2::ApplicationUIKit");

template <template <class, class> class Delegate> class ApplicationUIKitExec {
public:
  template <typename... DelegateArgs>
  static int exec(int argc, SystemChar** argv, SystemStringView appName,
                  DelegateArgs&&... args) noexcept {
    return ApplicationUIKit<Delegate>::exec(
        argc, argv, appName, std::forward<DelegateArgs>(args)...);
  }
};

} // namespace boo2
