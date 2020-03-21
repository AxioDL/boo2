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
  onAcceptDeviceRequest(const vk::PhysicalDeviceProperties& props) noexcept {
    return true;
  }
#endif

  void onStartBuildPipelines(std::size_t done, std::size_t count) noexcept {}
  void onUpdateBuildPipelines(std::size_t done, std::size_t count) noexcept {}
  void onEndBuildPipelines(std::size_t done, std::size_t count) noexcept {}
};

} // namespace boo2
