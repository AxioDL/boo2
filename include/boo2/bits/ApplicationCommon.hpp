#pragma once

#include "boo2/System.hpp"

#include <hsh/hsh.h>
#include <logvisor/logvisor.hpp>

#include <chrono>

namespace boo2 {

class WindowBase {
protected:
  hsh::resource_owner<hsh::surface> m_surface;
  WindowBase() noexcept = default;
  WindowBase(WindowBase&& other) noexcept
      : m_surface(std::move(other.m_surface)) {}
  WindowBase& operator=(WindowBase&& other) {
    m_surface = std::move(other.m_surface);
    return *this;
  }

public:
  bool acquireNextImage() noexcept { return m_surface.acquire_next_image(); }
  hsh::surface getSurface() const noexcept { return m_surface.get(); }
  operator hsh::surface() const noexcept { return getSurface(); }
};

class ApplicationBase {
protected:
  SystemStringView m_appName;

  explicit ApplicationBase(SystemStringView appName) noexcept
      : m_appName(appName) {}
};

#if HSH_ENABLE_VULKAN

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
    Log.report(vkSeverityToLevel(messageSeverity), fmt("Vulkan: {}"),
               pCallbackData.pMessage);
  }

public:
  explicit VulkanInstance(SystemStringView appName) noexcept
      : hsh::vulkan_instance_owner(hsh::create_vulkan_instance(
            appName.data(), 0, "boo2", 0, &handleError)) {}
};
logvisor::Module VulkanInstance::Log("boo2::VulkanInstance");

class VulkanDevice : public hsh::vulkan_device_owner {
public:
  VulkanDevice() noexcept = default;
  template <typename Delegate>
  VulkanDevice(const VulkanInstance& instance, Delegate& delegate,
               vk::SurfaceKHR checkSurface = {}) noexcept
      : hsh::vulkan_device_owner(instance.enumerate_vulkan_devices(
            [&](const vk::PhysicalDeviceProperties& props) {
              return delegate.onAcceptDeviceRequest(props);
            },
            checkSurface)) {}
};

template <class PCFM, class Delegate> class ApplicationVulkan {
protected:
  VulkanInstance m_instance;
  VulkanDevice m_device;

  hsh::vulkan_device_owner::pipeline_build_pump<PCFM> m_buildPump;

  bool pumpBuildVulkanPipelines(PCFM& pcfm, Delegate& delegate) noexcept {
    std::size_t done, count;

    if (!m_buildPump) {
      m_buildPump = m_device.start_build_pipelines(pcfm);
      std::tie(done, count) = m_buildPump.get_progress();
      delegate.onStartBuildPipelines(done, count);
      return count != 0;
    }

    // Build pipelines for approximately 100ms before another application cycle
    auto start = std::chrono::steady_clock::now();
    while (m_buildPump.pump()) {
      if (std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - start)
              .count() > 100) {
        std::tie(done, count) = m_buildPump.get_progress();
        delegate.onUpdateBuildPipelines(done, count);
        return true;
      }
    }

    std::tie(done, count) = m_buildPump.get_progress();
    delegate.onEndBuildPipelines(done, count);
    return false;
  }

  explicit ApplicationVulkan(SystemStringView appName) noexcept
      : m_instance(appName) {}
};

#endif

} // namespace boo2
