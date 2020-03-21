#include "testpipeline.hpp"
#include "testpipeline.cpp.hshhead"

using namespace hsh::pipeline;

struct DrawSomething : pipeline<color_attachment<>> {
  DrawSomething(hsh::dynamic_uniform_buffer<UniformData> u,
                hsh::vertex_buffer<MyFormat> v, hsh::texture2d<float> tex0) {
    position = u->xf * hsh::float4{v->position, 1.f};
    color_out[0] = hsh::float4{1.f, 1.f, 1.f, 1.f};
  }
};

Binding BuildPipeline() {
  auto uni = hsh::create_dynamic_uniform_buffer<UniformData>();
  std::array<MyFormat, 3> VtxData{MyFormat{hsh::float3{0.f, 0.f, 0.f}, {}},
                                  MyFormat{hsh::float3{1.f, 0.f, 0.f}, {}},
                                  MyFormat{hsh::float3{1.f, 1.f, 0.f}, {}}};
  auto vtx = hsh::create_vertex_buffer(VtxData);
  auto tex = hsh::create_texture2d<float>(
      {1024, 1024}, hsh::Format::RGBA8_UNORM, 10,
      [](void *buf, std::size_t size) { std::memset(buf, 0, size); });
  auto bind = hsh_DrawSomething(DrawSomething(uni.get(), vtx.get(), tex.get()));
  return {std::move(uni), std::move(vtx), std::move(tex), std::move(bind)};
}