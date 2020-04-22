#include <boo2/boo2.hpp>
#include <iostream>

#include "testpipeline.hpp"

using namespace std::literals;

template <class App, class Win>
class Delegate : public boo2::DelegateBase<App, Win> {
  Win m_window;
  hsh::owner<hsh::render_texture2d> m_renderTexture;
  Binding PipelineBind{};

public:
  void onAppLaunched(App& a) noexcept {
    std::cout << "launch" << std::endl;
    m_window = a.createWindow(_SYS_STR("Hello World!"sv), 0, 0, 512, 512);
    m_renderTexture = hsh::create_render_texture2d(m_window);
  }

  void onAppIdle(App& a) noexcept {
    if (!PipelineBind.Binding)
      PipelineBind = BuildPipeline();

    UniformData UniData{};
    UniData.xf[0][0] = 1.f;
    UniData.xf[1][1] = 1.f;
    UniData.xf[2][2] = 1.f;
    UniData.xf[3][3] = 1.f;
    PipelineBind.Uniform.load(UniData);

    if (m_window.acquireNextImage()) {
      m_renderTexture.attach();
      hsh::clear_attachments();
      PipelineBind.Binding.draw(0, 3);
      m_renderTexture.resolve_surface(m_window);
    }
  }

  void onAppExiting(App& a) noexcept { std::cout << "exiting" << std::endl; }

  void onQuitRequest(App& a) noexcept {
    std::cout << "quit" << std::endl;
    a.quit();
  }

#if HSH_ENABLE_VULKAN
  bool onAcceptDeviceRequest(
      App& a, const vk::PhysicalDeviceProperties& props,
      const vk::PhysicalDeviceDriverProperties& driverProps) noexcept {
    if (driverProps.driverID == vk::DriverId::eMesaRadv)
      return true;
    //if (driverProps.driverID == vk::DriverId::eAmdOpenSource)
    //  return true;
    return false;
  }
#endif

  void onStartBuildPipelines(App& a, std::size_t done,
                             std::size_t count) noexcept {
    std::cout << "start " << done << "/" << count << std::endl;
  }

  void onUpdateBuildPipelines(App& a, std::size_t done,
                              std::size_t count) noexcept {
    std::cout << "update " << done << "/" << count << std::endl;
  }

  void onEndBuildPipelines(App& a, std::size_t done,
                           std::size_t count) noexcept {
    std::cout << "end " << done << "/" << count << std::endl;
  }

  void onWindowResize(App& a, typename Win::ID id,
                      const hsh::extent2d& extent) noexcept {
    std::cout << "resize " << extent.w << "*" << extent.h << std::endl;
  }
};

int main(int argc, char** argv) noexcept {
  logvisor::RegisterConsoleLogger();
  return boo2::Application<Delegate>::exec(argc, argv, "boo2_testapp"sv);
}
