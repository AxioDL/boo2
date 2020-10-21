#pragma once

#include "WindowDecorations.hpp"
#include "boo2/System.hpp"

#include <hsh/hsh.h>
#include <logvisor/logvisor.hpp>

#include <chrono>

namespace boo2 {

extern std::vector<SystemString> Args;
extern SystemString AppName;
#ifdef BOO2_IMPLEMENTATION
std::vector<SystemString> Args;
SystemString AppName;
#endif

#if HSH_ENABLE_VULKAN && !HSH_PROFILE_MODE

class VulkanInstance : public hsh::vulkan_instance_owner {
  static logvisor::Module Log;

  static logvisor::Level vkSeverityToLevel(
      vk::DebugUtilsMessageSeverityFlagBitsEXT severity) noexcept {
    switch (severity) {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
      return logvisor::Error;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
      return logvisor::Warning;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
    default:
      return logvisor::Info;
    }
  }

  static void handleError(
      vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      vk::DebugUtilsMessageTypeFlagBitsEXT messageTypes,
      const vk::DebugUtilsMessengerCallbackDataEXT& pCallbackData) noexcept {
    Log.report(vkSeverityToLevel(messageSeverity), FMT_STRING("Vulkan: {}"),
               pCallbackData.pMessage);
  }

public:
  explicit VulkanInstance(SystemStringView appName) noexcept
      : hsh::vulkan_instance_owner(hsh::create_vulkan_instance(
            appName.data(), 0, "boo2", 0, &handleError)) {}
};
inline logvisor::Module VulkanInstance::Log("boo2::VulkanInstance");

class VulkanDevice : public hsh::vulkan_device_owner {
public:
  VulkanDevice() noexcept = default;
  template <typename App, typename Delegate>
  VulkanDevice(const VulkanInstance& instance, App& a, Delegate& delegate,
               vk::SurfaceKHR checkSurface = {}) noexcept
      : hsh::vulkan_device_owner(instance.enumerate_vulkan_devices(
            [&](const vk::PhysicalDeviceProperties& props,
                const vk::PhysicalDeviceDriverProperties& driverProps) {
              return delegate.onAcceptDeviceRequest(a, props, driverProps);
            },
            checkSurface)) {}
  template <class PCFM>
  using BuildPump = hsh::vulkan_device_owner::pipeline_build_pump<PCFM>;
};

struct VulkanTraits {
  using Instance = VulkanInstance;
  using Device = VulkanDevice;
};

#endif

#if HSH_ENABLE_METAL && !HSH_PROFILE_MODE

class MetalInstance : public hsh::metal_instance_owner {
  static logvisor::Module Log;

  static void handleError(NSError* error) noexcept {
    Log.report(logvisor::Fatal, FMT_STRING("Metal: {}"),
               error.localizedDescription.UTF8String);
  }

public:
  explicit MetalInstance(SystemStringView appName) noexcept
      : hsh::metal_instance_owner(hsh::create_metal_instance(&handleError)) {}
};
inline logvisor::Module MetalInstance::Log("boo2::MetalInstance");

class MetalDevice : public hsh::metal_device_owner {
public:
  MetalDevice() noexcept = default;
  template <typename App, typename Delegate>
  MetalDevice(const MetalInstance& instance, App& a, Delegate& delegate,
              CAMetalLayer* checkSurface = {}) noexcept
      : hsh::metal_device_owner(instance.enumerate_metal_devices(
            [&](id<MTLDevice> device, bool nativeForPhysSurface) {
              return delegate.onAcceptDeviceRequest(a, device,
                                                    nativeForPhysSurface);
            },
            checkSurface)) {}
  template <class PCFM>
  using BuildPump = hsh::metal_device_owner::pipeline_build_pump<PCFM>;
};

struct MetalTraits {
  using Instance = MetalInstance;
  using Device = MetalDevice;
};

#endif

#if HSH_ENABLE_DEKO3D && !HSH_PROFILE_MODE

class DekoInstance {
  static logvisor::Module Log;

public:
  explicit DekoInstance(SystemStringView appName) noexcept {}
};
inline logvisor::Module DekoInstance::Log("boo2::DekoInstance");

class DekoDevice : public hsh::deko_device_owner {
  static logvisor::Module Log;

  static void DekoErrorHandler(void*, const char* Context, DkResult Result,
                               const char* Message) noexcept {
    if (Result == DkResult_Success) {
      Log.report(logvisor::Warning, FMT_STRING("Warning from deko[{}]: {}"),
                 Context, Message);
    } else {
      Log.report(logvisor::Fatal, FMT_STRING("Error from deko[{}]: {}: {}"),
                 Context, Result, Message);
    }
  }

  static constexpr uint32_t CmdBufSize = 0x1000000;
  static constexpr uint32_t CopyCmdMemSize = 0x1000000;
  static void DekoAddMemoryHandler(const char* CmdBufName, DkCmdBuf Cmdbuf,
                                   size_t MinReqSize) noexcept {
    Log.report(logvisor::Fatal, FMT_STRING("{} out of memory; needs {}"),
               CmdBufName, MinReqSize);
  }

public:
  explicit DekoDevice() noexcept
      : hsh::deko_device_owner(hsh::create_deko_device(
            DekoErrorHandler, DkCmdBufAddMemFunc(DekoAddMemoryHandler),
            CmdBufSize, CopyCmdMemSize)) {}

  using BuildPump = hsh::deko_device_owner::pipeline_build_pump;
};
inline logvisor::Module DekoDevice::Log("boo2::DekoDevice");

struct DekoTraits {
  using Instance = DekoInstance;
  using Device = DekoDevice;
};

#endif

#if !HSH_PROFILE_MODE
template <class RHITraits
#if !HSH_ENABLE_DEKO3D
          , class PCFM
#endif
          >
#endif
class ApplicationBase {
protected:
#if !HSH_PROFILE_MODE
  typename RHITraits::Instance m_instance;
  typename RHITraits::Device m_device;
#if HSH_ENABLE_DEKO3D
  typename RHITraits::Device::BuildPump m_buildPump;
  bool m_startedBuildPump = false;
#else
  typename RHITraits::Device::template BuildPump<PCFM> m_buildPump;
#endif
#endif

  explicit ApplicationBase(SystemStringView appName) noexcept
#if !HSH_PROFILE_MODE
      : m_instance(appName)
#endif
  {
    AppName = appName;
  }

  explicit ApplicationBase(int argc, SystemChar** argv,
                           SystemStringView appName) noexcept
#if !HSH_PROFILE_MODE
      : m_instance(appName)
#endif
  {
    if (argc > 1) {
      Args.reserve(argc - 1);
      for (int i = 1; i < argc; ++i)
        Args.emplace_back(argv[i]);
    }
    AppName = appName;
  }

#if !HSH_PROFILE_MODE
  template <class App, class Delegate>
#if HSH_ENABLE_DEKO3D
  bool pumpBuildRHIPipelines(App& a, Delegate& delegate) noexcept {
#else
  bool pumpBuildRHIPipelines(PCFM& pcfm, App& a, Delegate& delegate) noexcept {
#endif
    std::size_t done, count;

#if HSH_ENABLE_DEKO3D
    if (!m_startedBuildPump) {
      m_buildPump = m_device.start_build_pipelines();
      m_startedBuildPump = true;
#else
    if (!m_buildPump) {
      m_buildPump = m_device.start_build_pipelines(pcfm);
#endif
      std::tie(done, count) = m_buildPump.get_progress();
      delegate.onStartBuildPipelines(a, done, count);
      return count != 0;
    }

    // Build pipelines for approximately 100ms before another application cycle
    auto start = std::chrono::steady_clock::now();
    while (m_buildPump.pump()) {
      if (std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - start)
              .count() > 100) {
        std::tie(done, count) = m_buildPump.get_progress();
        delegate.onUpdateBuildPipelines(a, done, count);
        return true;
      }
    }

    std::tie(done, count) = m_buildPump.get_progress();
    delegate.onEndBuildPipelines(a, done, count);
    return false;
  }
#endif
};

#undef DELETE
enum class Keycode {
#define BOO2_SPECIAL_KEYCODE(name, xkbcode, vkcode, maccode, ioscode) name,
#include "SpecialKeycodes.def"
};

enum class KeyModifier { None = 0, Control = 1, Alt = 2, Shift = 4 };
ENABLE_BITWISE_ENUM(KeyModifier)

enum class MouseButton {
#define BOO2_XBUTTON(name, xnum, wlnum) name,
#include "XButtons.def"
};

} // namespace boo2
