#pragma once

#include "ApplicationCommon.hpp"
#include <switch.h>

namespace boo2 {

template <class Application> class WindowNx {
  friend Application;
  hsh::owner<hsh::surface> m_hshSurface;
  Application* m_parent = nullptr;
  void* m_window = nullptr;

  WindowNx(Application* parent, int w, int h) noexcept : m_parent(parent) {
    m_window = nwindowGetDefault();
    m_hshSurface = hsh::create_surface(m_window, {},
                                       hsh::extent2d{uint32_t(w), uint32_t(h)});
  }

public:
  using ID = void*;
  operator void*() const noexcept { return m_window; }
  ID getID() const noexcept { return m_window; }
  bool operator==(ID id) const noexcept { return m_window == id; }
  bool operator!=(ID id) const noexcept { return m_window != id; }

  WindowNx() noexcept = default;

  WindowNx(WindowNx&& other) noexcept
      : m_hshSurface(std::move(other.m_hshSurface)), m_parent(other.m_parent),
        m_window(other.m_window) {
    other.m_parent = nullptr;
    other.m_window = nullptr;
  }

  WindowNx& operator=(WindowNx&& other) noexcept {
    std::swap(m_hshSurface, other.m_hshSurface);
    std::swap(m_parent, other.m_parent);
    std::swap(m_window, other.m_window);
    return *this;
  }

  WindowNx(const WindowNx&) = delete;
  WindowNx& operator=(const WindowNx&) = delete;

  bool acquireNextImage() noexcept { return m_hshSurface.acquire_next_image(); }
  hsh::surface getSurface() const noexcept { return m_hshSurface.get(); }
  operator hsh::surface() const noexcept { return getSurface(); }

  void setTitle(SystemStringView title) noexcept {}
};

template <template <class, class> class Delegate>
class ApplicationNx : public ApplicationBase {
  friend WindowNx<ApplicationNx<Delegate>>;
  using Window = WindowNx<ApplicationNx<Delegate>>;
  static logvisor::Module Log;

  hsh::deko_device_owner m_device;
  Delegate<ApplicationNx, Window> m_delegate;
  bool m_builtWindow = false;

public:
  [[nodiscard]] Window createWindow(SystemStringView title, int x, int y, int w,
                                    int h) noexcept {
    if (m_builtWindow)
      Log.report(logvisor::Fatal,
                 FMT_STRING("boo2 currently only supports one window"));

    if ((w != 1280 && w != 1920) || (h != 720 && h != 1080))
      Log.report(logvisor::Fatal,
                 FMT_STRING("nx only supports HD window dimensions"));

    Window window(this, w, h);
    if (!window)
      return {};

    m_buildingPipelines = true;
    m_builtWindow = true;
    return window;
  }

  void quit(int code = 0) noexcept {
    m_running = false;
    m_exitCode = code;
  }

private:
  hsh::deko_device_owner::pipeline_build_pump m_buildPump;
  int m_exitCode = 0;
  bool m_running = true;
  bool m_buildingPipelines = false;
  bool m_startedPipelineBuild = false;

  bool pumpBuildPipelines() noexcept {
    std::size_t done, count;

    if (!m_startedPipelineBuild) {
      m_startedPipelineBuild = true;
      m_buildPump = m_device.start_build_pipelines();
      std::tie(done, count) = m_buildPump.get_progress();
      m_delegate.onStartBuildPipelines(*this, done, count);
      return count != 0;
    }

    // Build pipelines for approximately 100ms before another application cycle
    auto start = std::chrono::steady_clock::now();
    while (m_buildPump.pump()) {
      if (std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - start)
              .count() > 100) {
        std::tie(done, count) = m_buildPump.get_progress();
        m_delegate.onUpdateBuildPipelines(*this, done, count);
        return true;
      }
    }

    std::tie(done, count) = m_buildPump.get_progress();
    m_delegate.onEndBuildPipelines(*this, done, count);
    return false;
  }

  int run() noexcept {
    if (!this->m_device)
      return 1;

    this->m_delegate.onAppLaunched(*this);

    while (m_running && appletMainLoop()) {
      hidScanInput();

      u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
      if (kDown & KEY_PLUS)
        break; // break in order to return to hbmenu

      this->m_device.enter_draw_context([this]() {
        if (m_buildingPipelines)
          m_buildingPipelines = this->pumpBuildPipelines();
        else
          this->m_delegate.onAppIdle(*this);
      });
    }

    this->m_delegate.onAppExiting(*this);

    if (this->m_device)
      this->m_device.wait_idle();
    return m_exitCode;
  }

  static void DekoErrorHandler(void*, const char* Context, DkResult Result,
                               const char* Message) noexcept {
    Log.report(logvisor::Fatal, FMT_STRING("Error from deko[{}]: {}: {}"),
               Context, Result, Message);
  }

  static constexpr uint32_t CmdBufSize = 0x10000;
  static constexpr uint32_t CopyCmdMemSize = 0x4000;
  static void DekoAddMemoryHandler(const char* CmdBufName, DkCmdBuf Cmdbuf,
                                   size_t MinReqSize) noexcept {
    Log.report(logvisor::Fatal, FMT_STRING("{} out of memory; needs {}"),
               CmdBufName, MinReqSize);
  }

  template <typename... DelegateArgs>
  explicit ApplicationNx(int argc, SystemChar** argv, SystemStringView appName,
                         DelegateArgs&&... args) noexcept
      : ApplicationBase(argc, argv, appName),
        m_delegate(std::forward<DelegateArgs>(args)...) {
    m_device = hsh::create_deko_device(DekoErrorHandler,
                                       DkCmdBufAddMemFunc(DekoAddMemoryHandler),
                                       CmdBufSize, CopyCmdMemSize);
  }

public:
  ApplicationNx(ApplicationNx&&) = delete;
  ApplicationNx& operator=(ApplicationNx&&) = delete;
  ApplicationNx(const ApplicationNx&) = delete;
  ApplicationNx& operator=(const ApplicationNx&) = delete;

  template <typename... DelegateArgs>
  static int exec(int argc, SystemChar** argv, SystemStringView appName,
                  DelegateArgs&&... args) noexcept {
    ApplicationNx app(argc, argv, appName, std::forward<DelegateArgs>(args)...);
    return app.run();
  }
};
template <template <class, class> class Delegate>
logvisor::Module ApplicationNx<Delegate>::Log("boo2::ApplicationNx");

template <template <class, class> class Delegate> class ApplicationNxExec {
  static logvisor::Module Log;

public:
  template <typename... DelegateArgs>
  static int exec(int argc, SystemChar** argv, SystemStringView appName,
                  DelegateArgs&&... args) noexcept {
    return ApplicationNx<Delegate>::exec(argc, argv, appName,
                                         std::forward<DelegateArgs>(args)...);
  }
};
template <template <class, class> class Delegate>
logvisor::Module ApplicationNxExec<Delegate>::Log("boo2::ApplicationNxExec");

} // namespace boo2
