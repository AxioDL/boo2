#include "testpipeline.hpp"
#include "testpipeline.cpp.hshhead"

using namespace hsh::pipeline;

struct DrawSomething : pipeline<color_attachment<>> {
  DrawSomething(hsh::uniform_buffer<UniformData> u,
                hsh::vertex_buffer<MyFormat> v, hsh::texture2d tex0) {
    position = u->xf * hsh::float4{v->position, 1.f};
    color_out[0] = v->color;
  }
};

hsh::binding& Binding::Bind(hsh::uniform_buffer<UniformData> u,
                   hsh::vertex_buffer<MyFormat> v) {
  if (!Tex) {
    Tex = hsh::create_texture2d(
        {1024, 1024}, hsh::Format::RGBA8_UNORM, 10,
        [](void* buf, std::size_t size) { std::memset(buf, 0, size); });
  }
  Binding.hsh_DrawSomething(DrawSomething(u, v, Tex.get()));
  return Binding;
}
