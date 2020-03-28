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
  bool onAcceptDeviceRequest(
      App& a, const vk::PhysicalDeviceProperties& props,
      const vk::PhysicalDeviceDriverProperties& driverProps) noexcept {
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

  void onUtf32Pressed(App& a, typename Win::ID id, uint32_t ch,
                      KeyModifier mods) noexcept {}
  void onUtf32Released(App& a, typename Win::ID id, uint32_t ch,
                       KeyModifier mods) noexcept {}
  void onSpecialKeyPressed(App& a, typename Win::ID id, Keycode key,
                           KeyModifier mods) noexcept {}
  void onSpecialKeyReleased(App& a, typename Win::ID id, Keycode key,
                            KeyModifier mods) noexcept {}

  void onMouseDown(App& a, typename Win::ID id, const hsh::offset2dF& offset,
                   MouseButton button, KeyModifier mods) noexcept {}
  void onMouseUp(App& a, typename Win::ID id, const hsh::offset2dF& offset,
                 MouseButton button, KeyModifier mods) noexcept {}
  void onMouseMove(App& a, typename Win::ID id, const hsh::offset2dF& offset,
                   KeyModifier mods) noexcept {}
  void onScroll(App& a, typename Win::ID id, const hsh::offset2dF& delta,
                KeyModifier mods) noexcept {}

  void onMouseEnter(App& a, typename Win::ID id, const hsh::offset2dF& offset,
                    KeyModifier mods) noexcept {}
  void onMouseLeave(App& a, typename Win::ID id, const hsh::offset2dF& offset,
                    KeyModifier mods) noexcept {}
};

} // namespace boo2
