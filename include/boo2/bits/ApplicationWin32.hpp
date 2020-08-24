#pragma once

#include "ApplicationCommon.hpp"

#include <Shlobj.h>
#include <dwmapi.h>
#include <windowsx.h>

namespace boo2 {

template <class Application> class WindowWin32 {
  friend Application;
  static constexpr int32_t MinDim = 256;
  static constexpr int32_t MaxDim = 16384;
  static constexpr DWORD WindowedStyle = WS_OVERLAPPEDWINDOW;
  static constexpr DWORD WindowedExStyle = WS_EX_APPWINDOW;
  static constexpr DWORD FullscreenStyle = WS_POPUP;
  static constexpr DWORD FullscreenExStyle = WS_EX_APPWINDOW;
  hsh::owner<hsh::surface> m_hshSurface;
  Application* m_parent = nullptr;
  HWND m_hwnd = nullptr;
  std::unique_ptr<WindowDecorations> m_decorations;

  explicit WindowWin32(Application* parent, SystemStringView title, int x,
                       int y, int w, int h) noexcept
      : m_parent(parent), m_decorations(new WindowDecorations()) {
    RECT Rect{0, 0, w, h};
    AdjustWindowRectEx(&Rect, WindowedStyle, false, WindowedExStyle);
    m_hwnd = CreateWindowEx(
        WindowedExStyle, TEXT("boo2_window_class"), title.data(), WindowedStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, Rect.right - Rect.left,
        Rect.bottom - Rect.top, nullptr, nullptr, *parent, nullptr);
    if (!m_hwnd) {
      Application::Log.report(logvisor::Error,
                              FMT_STRING("Unable to create window"));
      return;
    }

    DWM_BLURBEHIND bb{};
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.fEnable = true;
    bb.hRgnBlur = CreateRectRgn(0, 0, -1, -1);
    DwmEnableBlurBehindWindow(m_hwnd, &bb);

    TRACKMOUSEEVENT TME = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hwnd,
                           HOVER_DEFAULT};
    TrackMouseEvent(&TME);
  }

  bool createSurface(
      vk::UniqueSurfaceKHR&& physSurface,
      std::function<void(const hsh::extent2d&)>&& resizeLambda) noexcept {
    m_hshSurface = hsh::create_surface(
        std::move(physSurface),
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

public:
  using ID = HWND;
  operator HWND() const noexcept { return m_hwnd; }
  ID getID() const noexcept { return m_hwnd; }
  bool operator==(ID hwnd) const noexcept { return m_hwnd == hwnd; }
  bool operator!=(ID hwnd) const noexcept { return m_hwnd != hwnd; }

  WindowWin32() noexcept = default;

  ~WindowWin32() noexcept { DestroyWindow(m_hwnd); }

  WindowWin32(WindowWin32&& other) noexcept
      : m_hshSurface(std::move(other.m_hshSurface)), m_parent(other.m_parent),
        m_hwnd(other.m_hwnd), m_decorations(std::move(other.m_decorations)) {
    other.m_parent = nullptr;
    other.m_hwnd = nullptr;
  }

  WindowWin32& operator=(WindowWin32&& other) noexcept {
    std::swap(m_hshSurface, other.m_hshSurface);
    std::swap(m_parent, other.m_parent);
    std::swap(m_hwnd, other.m_hwnd);
    std::swap(m_decorations, other.m_decorations);
    return *this;
  }

  WindowWin32(const WindowWin32&) = delete;
  WindowWin32& operator=(const WindowWin32&) = delete;

  bool acquireNextImage() noexcept { return m_hshSurface.acquire_next_image(); }
  hsh::surface getSurface() const noexcept { return m_hshSurface.get(); }
  operator hsh::surface() const noexcept { return getSurface(); }

  void setTitle(SystemStringView title) noexcept {
    SetWindowText(m_hwnd, title.data());
  }
};

class Win32Paths {
  SystemString m_config;
  SystemString m_data;
  SystemString m_cache;

  static SystemString getLocalAppDataPath(SystemStringView appName,
                                          const SystemChar* subdir) noexcept {
    PWSTR localAppData = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr,
                             &localAppData) != S_OK)
      return {};
#ifdef UNICODE
    PTSTR localAppDataT = localAppData;
#else
    SystemString localAppDataA;
    int localAppDataLen = int(std::wcslen(localAppData));
    int length = WideCharToMultiByte(CP_UTF8, 0, localAppData, localAppDataLen,
                                     nullptr, 0, nullptr, nullptr);
    localAppDataA.resize(length);
    WideCharToMultiByte(CP_UTF8, 0, localAppData, localAppDataLen,
                        &localAppDataA[0], length, nullptr, nullptr);
    PTSTR localAppDataT = localAppDataA.data();
#endif

    SystemString appRoot =
        fmt::format(FMT_STRING(TEXT("{}\\{}")), localAppDataT, appName);
    CreateDirectory(appRoot.c_str(), nullptr);

    SystemString ret = fmt::format(FMT_STRING(TEXT("{}\\{}\\{}")),
                                   localAppDataT, appName, subdir);
    CoTaskMemFree(localAppData);
    CreateDirectory(ret.c_str(), nullptr);
    return ret;
  }

public:
  explicit Win32Paths(SystemStringView appName) noexcept {
    m_config = getLocalAppDataPath(appName, TEXT("config"));
    m_data = getLocalAppDataPath(appName, TEXT("data"));
    m_cache = getLocalAppDataPath(appName, TEXT("cache"));
  }

  SystemString getConfigPath(SystemStringView subPath) const noexcept {
    return fmt::format(FMT_STRING(TEXT("{}/{}")), m_config, subPath);
  }

  SystemString getDataPath(SystemStringView subPath) const noexcept {
    return fmt::format(FMT_STRING(TEXT("{}/{}")), m_data, subPath);
  }

  SystemString getCachePath(SystemStringView subPath) const noexcept {
    return fmt::format(FMT_STRING(TEXT("{}/{}")), m_cache, subPath);
  }
};

struct Win32PipelineCacheFileManager {
  static logvisor::Module Log;
  const Win32Paths& m_paths;

  explicit Win32PipelineCacheFileManager(Win32Paths& paths) noexcept
      : m_paths(paths) {}

  std::string GetFilename(const uint8_t UUID[VK_UUID_SIZE]) noexcept {
    std::ostringstream FileName;
    FileName << "hsh-test-pipeline-cache-";
    for (int i = 0; i < VK_UUID_SIZE; ++i)
      FileName << std::hex << unsigned(UUID[i]);
    FileName << ".bin";
    return m_paths.getCachePath(FileName.str());
  }

  template <typename Func>
  void ReadPipelineCache(Func F, const uint8_t UUID[VK_UUID_SIZE]) noexcept {
    auto path = GetFilename(UUID);
    HANDLE File =
        CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (File != INVALID_HANDLE_VALUE) {
      LARGE_INTEGER Size{};
      GetFileSizeEx(File, &Size);
      if (Size.QuadPart != 0) {
        std::unique_ptr<uint8_t[]> Data(new uint8_t[Size.QuadPart]);
        DWORD SizeRead = 0;
        if (ReadFile(File, Data.get(), Size.LowPart, &SizeRead, nullptr))
          F(Data.get(), SizeRead);
      }
      CloseHandle(File);
    } else {
      Log.report(logvisor::Warning,
                 FMT_STRING("Unable to open {} for reading."), path);
    }
  }

  template <typename Func>
  void WritePipelineCache(Func F, const uint8_t UUID[VK_UUID_SIZE]) noexcept {
    auto path = GetFilename(UUID);
    HANDLE File =
        CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr,
                   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (File != INVALID_HANDLE_VALUE) {
      F([File](const uint8_t* Data, std::size_t Size) {
        DWORD BytesWritten = 0;
        WriteFile(File, Data, DWORD(Size), &BytesWritten, nullptr);
      });
      CloseHandle(File);
    } else {
      Log.report(logvisor::Warning,
                 FMT_STRING("Unable to open {} for writing."), path);
    }
  }
};
inline logvisor::Module
    Win32PipelineCacheFileManager::Log("boo2::Win32PipelineCacheFileManager");

template <template <class, class> class Delegate>
class ApplicationWin32
    : public ApplicationBase,
      public ApplicationVulkan<Win32PipelineCacheFileManager> {
  Delegate<ApplicationWin32<Delegate>, WindowWin32<ApplicationWin32<Delegate>>>
      m_delegate;
  Win32Paths m_winPaths;
  Win32PipelineCacheFileManager m_pcfm{m_winPaths};
  friend WindowWin32<ApplicationWin32<Delegate>>;
  using Window = WindowWin32<ApplicationWin32<Delegate>>;
  static logvisor::Module Log;

  HINSTANCE m_hinstance;
  operator HINSTANCE() const noexcept { return m_hinstance; }

  bool m_builtWindow = false;

  struct WindowData {
    ApplicationWin32* m_parent;
    LONG_PTR m_mouseEntered;
    LONG_PTR m_windowedLeft;
    LONG_PTR m_windowedTop;
    LONG_PTR m_windowedRight;
    LONG_PTR m_windowedBottom;
  };
#define SetWindowDataField(hwnd, field, data)                                  \
  SetWindowLongPtr(hwnd, offsetof(WindowData, field), (LONG_PTR)(data))
#define GetWindowDataField(hwnd, field)                                        \
  ((decltype(WindowData::field))(                                              \
      GetWindowLongPtr(hwnd, offsetof(WindowData, field))))

  static void SetWindowedRect(HWND hwnd, const RECT& rect) noexcept {
    SetWindowDataField(hwnd, m_windowedLeft, rect.left);
    SetWindowDataField(hwnd, m_windowedTop, rect.top);
    SetWindowDataField(hwnd, m_windowedRight, rect.right);
    SetWindowDataField(hwnd, m_windowedBottom, rect.bottom);
  }

  static RECT GetWindowedRect(HWND hwnd) noexcept {
    return {LONG(GetWindowDataField(hwnd, m_windowedLeft)),
            LONG(GetWindowDataField(hwnd, m_windowedTop)),
            LONG(GetWindowDataField(hwnd, m_windowedRight)),
            LONG(GetWindowDataField(hwnd, m_windowedBottom))};
  }

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
    SetWindowDataField(window, m_parent, this);

    RECT windowRect{};
    GetWindowRect(window, &windowRect);
    SetWindowedRect(window, windowRect);

    auto physSurface =
        this->m_instance.create_phys_surface(m_hinstance, window);
    if (!physSurface) {
      Log.report(logvisor::Error,
                 FMT_STRING("Unable to create XCB Vulkan surface"));
      return {};
    }

    if (!this->m_device) {
      this->m_device =
          VulkanDevice(this->m_instance, *this, this->m_delegate, *physSurface);
      if (!this->m_device) {
        Log.report(logvisor::Error,
                   FMT_STRING("No valid Vulkan devices accepted"));
        return {};
      }
      m_buildingPipelines = true;
    }

    HWND windowHandle = window.getID();
    if (!window.createSurface(
            std::move(physSurface),
            [this, windowHandle](const hsh::extent2d& extent) {
              this->m_delegate.onWindowResize(*this, windowHandle, extent);
            })) {
      Log.report(
          logvisor::Error,
          FMT_STRING("Vulkan surface not compatible with existing device"));
      return {};
    }

    ShowWindow(windowHandle, SW_SHOW);

    m_builtWindow = true;
    return window;
  }

  void dispatchLatestEvents() noexcept {
    // This particular version of the message loop is only interested in
    // direct keyboard and mouse events.
    MSG Msg;
    while (PeekMessage(&Msg, nullptr, WM_KEYFIRST, WM_MOUSELAST, PM_REMOVE)) {
      DispatchMessage(&Msg);
      if (Msg.message == WM_QUIT)
        quit(int(Msg.wParam));
    }
  }

  void quit(int code = 0) noexcept {
    m_running = false;
    m_exitCode = code;
  }

private:
  int m_exitCode = 0;
  int m_minWindowDimX = Window::MinDim;
  int m_minWindowDimY = Window::MinDim;
  bool m_running = true;
  bool m_inIdleFunc = false;
  bool m_buildingPipelines = false;

  void toggleFullscreen(HWND hwnd) noexcept {
    LONG CurStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (CurStyle & WS_POPUP) {
      // Make windowed
      RECT WindowRect = GetWindowedRect(hwnd);
      LONG Style = GetWindowLong(hwnd, GWL_STYLE);
      LONG ExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
      Style &= ~Window::FullscreenStyle;
      Style |= Window::WindowedStyle;
      ExStyle &= ~Window::FullscreenExStyle;
      ExStyle |= Window::WindowedExStyle;
      SetWindowLong(hwnd, GWL_STYLE, Style);
      SetWindowLong(hwnd, GWL_EXSTYLE, ExStyle);
      MoveWindow(hwnd, WindowRect.left, WindowRect.top,
                 WindowRect.right - WindowRect.left,
                 WindowRect.bottom - WindowRect.top, true);
    } else {
      // Make fullscreen
      HMONITOR Monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
      MONITORINFO MonitorInfo{sizeof(MONITORINFO)};
      if (GetMonitorInfo(Monitor, &MonitorInfo)) {
        RECT WindowRect{};
        GetWindowRect(hwnd, &WindowRect);
        SetWindowedRect(hwnd, WindowRect);
        LONG Style = GetWindowLong(hwnd, GWL_STYLE);
        LONG ExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        Style &= ~Window::WindowedStyle;
        Style |= Window::FullscreenStyle;
        ExStyle &= ~Window::WindowedExStyle;
        ExStyle |= Window::FullscreenExStyle;
        SetWindowLong(hwnd, GWL_STYLE, Style);
        SetWindowLong(hwnd, GWL_EXSTYLE, ExStyle);
        MoveWindow(hwnd, MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                   MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                   MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                   true);
      }
    }
  }

  static void getMods(KeyModifier& mods) noexcept {
    mods |= (USHORT(GetKeyState(VK_CONTROL)) >> 15u) == 1 ? KeyModifier::Control
                                                          : KeyModifier::None;
    mods |= (USHORT(GetKeyState(VK_MENU)) >> 15u) == 1 ? KeyModifier::Alt
                                                       : KeyModifier::None;
    mods |= (USHORT(GetKeyState(VK_SHIFT)) >> 15u) == 1 ? KeyModifier::Shift
                                                        : KeyModifier::None;
  }

  template <typename UTF32Func, typename SpecialFunc>
  static void translateVirtualKey(WPARAM wParam, UTF32Func utfFunc,
                                  SpecialFunc specialFunc) noexcept {
    KeyModifier mods = KeyModifier::None;
    getMods(mods);
    switch (wParam) {
#define BOO2_SPECIAL_KEYCODE(name, xkbcode, vkcode)                            \
  case vkcode:                                                                 \
    specialFunc(Keycode::name, mods);                                          \
    break;
#include "SpecialKeycodes.def"
    default:
      if (UINT charCode = MapVirtualKey(UINT(wParam), MAPVK_VK_TO_CHAR))
        utfFunc(charCode, mods);
      break;
    }
  }

  static MouseButton translateMouseButton(UINT uMsg, WPARAM wParam) noexcept {
    switch (uMsg) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    default:
      return MouseButton::Primary;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
      return MouseButton::Secondary;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
      return MouseButton::Middle;
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
      if (HIWORD(wParam) == XBUTTON1)
        return MouseButton::Aux1;
      else
        return MouseButton::Aux2;
    }
  }

  LRESULT _windowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                      LPARAM lParam) noexcept {
    switch (uMsg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    case WM_SIZE:
      // Trigger WM_PAINT on window resizes ASAP
      InvalidateRect(hwnd, nullptr, false);
      return 0;
    case WM_PAINT:
      // During window resizing, Win32 enters a modal message loop where the
      // best way to perform intermediate render dispatches is via WM_PAINT
      dispatchIdleFunc();
      ValidateRect(hwnd, nullptr);
      return 0;
    case WM_GETMINMAXINFO: {
      LPMINMAXINFO lpMMI = LPMINMAXINFO(lParam);
      lpMMI->ptMinTrackSize.x = m_minWindowDimX;
      lpMMI->ptMinTrackSize.y = m_minWindowDimY;
      lpMMI->ptMaxTrackSize.x = Window::MaxDim;
      lpMMI->ptMaxTrackSize.y = Window::MaxDim;
      return 0;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      translateVirtualKey(
          wParam,
          [a = this, w = hwnd](uint32_t sym, KeyModifier mods) {
            if (sym == '\r' && (mods & KeyModifier::Alt) != KeyModifier::None)
              a->toggleFullscreen(w);
            a->m_delegate.onUtf32Pressed(*a, w, sym, mods);
          },
          [a = this, w = hwnd](Keycode code, KeyModifier mods) {
            a->m_delegate.onSpecialKeyPressed(*a, w, code, mods);
          });
      return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP:
      translateVirtualKey(
          wParam,
          [a = this, w = hwnd](uint32_t sym, KeyModifier mods) {
            a->m_delegate.onUtf32Released(*a, w, sym, mods);
          },
          [a = this, w = hwnd](Keycode code, KeyModifier mods) {
            a->m_delegate.onSpecialKeyReleased(*a, w, code, mods);
          });
      return 0;
    case WM_MOUSELEAVE: {
      KeyModifier mods = KeyModifier::None;
      getMods(mods);
      this->m_delegate.onMouseLeave(*this, hwnd, hsh::offset2d(), mods);
      SetWindowDataField(hwnd, m_mouseEntered, false);
      return 0;
    }
    case WM_MOUSEMOVE: {
      KeyModifier mods = KeyModifier::None;
      getMods(mods);
      if (!GetWindowDataField(hwnd, m_mouseEntered)) {
        SetWindowDataField(hwnd, m_mouseEntered, true);
        this->m_delegate.onMouseEnter(
            *this, hwnd,
            hsh::offset2d(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), mods);
      }
      this->m_delegate.onMouseMove(
          *this, hwnd,
          hsh::offset2d(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), mods);
      return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN: {
      KeyModifier mods = KeyModifier::None;
      getMods(mods);
      this->m_delegate.onMouseDown(
          *this, hwnd,
          hsh::offset2d(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)),
          translateMouseButton(uMsg, wParam), mods);
      return 0;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP: {
      KeyModifier mods = KeyModifier::None;
      getMods(mods);
      this->m_delegate.onMouseUp(
          *this, hwnd,
          hsh::offset2d(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)),
          translateMouseButton(uMsg, wParam), mods);
      return 0;
    }
    case WM_MOUSEWHEEL: {
      KeyModifier mods = KeyModifier::None;
      getMods(mods);
      this->m_delegate.onScroll(
          *this, hwnd, hsh::offset2d(0, GET_WHEEL_DELTA_WPARAM(wParam)), mods);
      return 0;
    }
    case WM_MOUSEHWHEEL: {
      KeyModifier mods = KeyModifier::None;
      getMods(mods);
      this->m_delegate.onScroll(
          *this, hwnd, hsh::offset2d(GET_WHEEL_DELTA_WPARAM(wParam), 0), mods);
      return 0;
    }
    default:
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
  }

  static LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam) noexcept {
    if (ApplicationWin32* app = GetWindowDataField(hwnd, m_parent))
      return app->_windowProc(hwnd, uMsg, wParam, lParam);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }

#undef SetWindowDataField
#undef GetWindowDataField

  bool pumpBuildPipelines() noexcept {
    return this->pumpBuildVulkanPipelines(m_pcfm, *this, m_delegate);
  }

  void dispatchIdleFunc() noexcept {
    // Ideally this shouldn't happen, but prevent reentrancy as a last resort
    if (m_inIdleFunc)
      return;
    m_inIdleFunc = true;
    if (this->m_device) {
      this->m_device.enter_draw_context([this]() {
        if (m_buildingPipelines)
          m_buildingPipelines = this->pumpBuildPipelines();
        else
          this->m_delegate.onAppIdle(*this);
      });
    }
    m_inIdleFunc = false;
  }

  int run() noexcept {
    if (!this->m_instance)
      return 1;

    this->m_delegate.onAppLaunched(*this);

    while (m_running) {
      dispatchIdleFunc();
      MSG Msg;
      while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE)) {
        DispatchMessage(&Msg);
        if (Msg.message == WM_QUIT)
          quit(int(Msg.wParam));
      }
    }

    this->m_delegate.onAppExiting(*this);

    if (this->m_device)
      this->m_device.wait_idle();
    return m_exitCode;
  }

  template <typename... DelegateArgs>
  explicit ApplicationWin32(int argc, SystemChar** argv,
                            SystemStringView appName,
                            DelegateArgs&&... args) noexcept
      : ApplicationBase(argc, argv, appName),
        ApplicationVulkan<Win32PipelineCacheFileManager>(appName),
        m_winPaths(appName), m_hinstance(GetModuleHandle(nullptr)) {

    WNDCLASSEX WndClass = {sizeof(WNDCLASSEX),
                           0,
                           WNDPROC(&windowProc),
                           0,
                           sizeof(WindowData),
                           m_hinstance,
                           nullptr,
                           nullptr,
                           HBRUSH(GetStockObject(NULL_BRUSH)),
                           nullptr,
                           TEXT("boo2_window_class"),
                           nullptr};
    RegisterClassEx(&WndClass);

    RECT MinRect{0, 0, Window::MinDim, Window::MinDim};
    AdjustWindowRectEx(&MinRect, Window::WindowedStyle, false,
                       Window::WindowedExStyle);
    m_minWindowDimX = MinRect.right - MinRect.left;
    m_minWindowDimY = MinRect.bottom - MinRect.top;
  }

public:
  ApplicationWin32(ApplicationWin32&&) = delete;
  ApplicationWin32& operator=(ApplicationWin32&&) = delete;
  ApplicationWin32(const ApplicationWin32&) = delete;
  ApplicationWin32& operator=(const ApplicationWin32&) = delete;

  ~ApplicationWin32() noexcept { WindowDecorations::shutdown(); }

  template <typename... DelegateArgs>
  static int exec(int argc, SystemChar** argv, SystemStringView appName,
                  DelegateArgs&&... args) noexcept {
    ApplicationWin32 app(argc, argv, appName,
                         std::forward<DelegateArgs>(args)...);
    return app.run();
  }
};
template <template <class, class> class Delegate>
logvisor::Module ApplicationWin32<Delegate>::Log("boo2::ApplicationWin32");

template <template <class, class> class Delegate> class ApplicationWin32Exec {
public:
  template <typename... DelegateArgs>
  static int exec(int argc, SystemChar** argv, SystemStringView appName,
                  DelegateArgs&&... args) noexcept {
    return ApplicationWin32<Delegate>::exec(
        argc, argv, appName, std::forward<DelegateArgs>(args)...);
  }
};

} // namespace boo2
