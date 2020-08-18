#include <boo2/boo2.hpp>
#include <iostream>

#include "testpipeline.hpp"

using namespace std::literals;

template <class App, class Win>
class Delegate : public boo2::DelegateBase<App, Win> {
  Win m_window;
  hsh::owner<hsh::render_texture2d> m_renderTexture;
  hsh::uniform_fifo m_uFifo;
  hsh::vertex_fifo m_vFifo;
  Binding m_pipelineBind{};
  std::size_t CurColor = 0;

#if __SWITCH__
  static constexpr int W = 1280;
  static constexpr int H = 720;
#else
  static constexpr int W = 512;
  static constexpr int H = 512;
#endif

public:
  void onAppLaunched(App& a) noexcept {
    std::cout << "launch" << std::endl;
    m_window = a.createWindow(_SYS_STR("Hello World!"sv), 0, 0, W, H);
    if (!m_window) {
      a.quit(1);
      return;
    }
    m_renderTexture = hsh::create_render_texture2d(m_window);

    m_uFifo = hsh::create_uniform_fifo(sizeof(UniformData) + 256);
    m_vFifo = hsh::create_vertex_fifo(sizeof(MyFormat) * 3 * 2);
  }

  void onAppIdle(App& a) noexcept {
    if (m_window.acquireNextImage()) {
      a.dispatchLatestEvents();
      m_renderTexture.attach();
      hsh::clear_attachments();

      constexpr std::array<hsh::float4, 7> Rainbow{{{1.f, 0.f, 0.f, 1.f},
                                                       {1.f, 0.5f, 0.f, 1.f},
                                                       {1.f, 1.f, 0.f, 1.f},
                                                       {0.f, 1.f, 0.f, 1.f},
                                                       {0.f, 1.f, 1.f, 1.f},
                                                       {0.f, 0.f, 1.f, 1.f},
                                                       {0.5f, 0.f, 1.f, 1.f}}};

      auto uData = m_uFifo.map<UniformData>([](UniformData& Data) {
        Data = UniformData{};
        Data.xf[0][0] = 1.f;
        Data.xf[1][1] = 1.f;
        Data.xf[2][2] = 1.f;
        Data.xf[3][3] = 1.f;
      });
      auto vData = m_vFifo.map<MyFormat>(3, [&](MyFormat* Data) {
        Data[0] = MyFormat{hsh::float3{-1.f, -1.f, 0.f}, Rainbow[CurColor]};
        Data[1] = MyFormat{hsh::float3{ 1.f, -1.f, 0.f}, Rainbow[(CurColor + 1) % Rainbow.size()]};
        Data[2] = MyFormat{hsh::float3{ 1.f,  1.f, 0.f}, Rainbow[(CurColor + 2) % Rainbow.size()]};
      });
      CurColor = (CurColor + 1) % Rainbow.size();
      m_pipelineBind.Bind(uData, vData).draw(0, 3);

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
    return true;
    if (driverProps.driverID == vk::DriverId::eAmdOpenSource)
      return false;
    return true;
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
  logvisor::RegisterStandardExceptions();
  logvisor::RegisterConsoleLogger();
  return boo2::Application<Delegate>::exec(argc, argv, "boo2_testapp"sv);
}
