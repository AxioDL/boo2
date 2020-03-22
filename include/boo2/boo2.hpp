#pragma once

#include "System.hpp"

#ifdef _WIN32
#elif __APPLE__
#else
#include "bits/ApplicationPosix.hpp"
#endif

namespace boo2 {

#ifdef _WIN32
#elif __APPLE__
#else
template <template <class, class> class Delegate>
using Application = ApplicationPosixExec<Delegate>;
#endif

template <class App, class Win> class DelegateBase {
public:
  void onAppLaunched(App& a) noexcept {}
  void onAppIdle(App& a) noexcept {}
  void onAppExiting(App& a) noexcept {}
  void onQuitRequest(App& a) noexcept { a.quit(); }

#if HSH_ENABLE_VULKAN
  bool
  onAcceptDeviceRequest(App& a,
                        const vk::PhysicalDeviceProperties& props) noexcept {
    return true;
  }
#endif

  void onStartBuildPipelines(App& a, std::size_t done,
                             std::size_t count) noexcept {}
  void onUpdateBuildPipelines(App& a, std::size_t done,
                              std::size_t count) noexcept {}
  void onEndBuildPipelines(App& a, std::size_t done,
                           std::size_t count) noexcept {}

  void onWindowResize(App& a, typename Win::ID id,
                      const hsh::extent2d& extent) noexcept {}
};

} // namespace boo2
