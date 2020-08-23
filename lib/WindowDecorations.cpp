#include "boo2/bits/WindowDecorations.hpp"
#include "WindowDecorations.cpp.hshhead"

namespace boo2 {

extern const struct WindowDecorationsRes {
  unsigned int width;
  unsigned int height;
  unsigned int bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
} window_decorations;
extern const unsigned char window_decorations_data[];

using namespace hsh::pipeline;

struct DecorationPipeline
    : pipeline<color_attachment<hsh::SrcAlpha, hsh::Src1Alpha, hsh::Add,
                                hsh::One, hsh::Src1Alpha, hsh::Add>,
               depth_write<false>, direct_render, dual_source, high_priority> {
  DecorationPipeline(hsh::vertex_buffer<WindowDecorations::Vert> vbo,
                     hsh::index_buffer<uint16_t> ibo, hsh::texture2d tex) {
    position = hsh::float4(vbo->pos, 0.f, 1.f);
    hsh::float4 texel = tex.sample<float>(
        vbo->uv,
        hsh::sampler(hsh::Linear, hsh::Linear, hsh::Linear, hsh::ClampToEdge,
                     hsh::ClampToEdge, hsh::ClampToEdge));
    color_out[0] = hsh::float4(texel.x, texel.x, texel.x, texel.z);
    color_out[1] = hsh::float4(0.f, 0.f, 0.f, texel.y);
  }
};

constexpr uint16_t CornerBase(uint8_t corner) { return corner * 4; }
constexpr uint16_t CornerEdgeH(uint8_t corner) { return corner * 2 + 16; }
constexpr uint16_t CornerEdgeV(uint8_t corner) { return corner * 2 + 16 + 1; }

template <std::size_t I>
constexpr uint16_t ExpandQuad(std::array<uint16_t, 4> arr) {
  return std::get<I>(arr);
}

template <std::size_t... QSeq>
constexpr std::array<uint16_t, 48> GenIndices(std::index_sequence<QSeq...>) {
  return {(QSeq + 0)...,  // NW
          (QSeq + 4)...,  // NE
          (QSeq + 8)...,  // SW
          (QSeq + 12)..., // SE
          // N
          ExpandQuad<QSeq>({CornerBase(0) + 2, CornerEdgeH(0),
                            CornerBase(1) + 0, CornerEdgeH(1)})...,
          // S
          ExpandQuad<QSeq>({CornerEdgeH(2), CornerBase(2) + 3, CornerEdgeH(3),
                            CornerBase(3) + 1})...,
          // W
          ExpandQuad<QSeq>({CornerBase(2) + 0, CornerEdgeV(2),
                            CornerBase(0) + 1, CornerEdgeV(0)})...,
          // E
          ExpandQuad<QSeq>({CornerEdgeV(3), CornerBase(3) + 2, CornerEdgeV(1),
                            CornerBase(1) + 3})...};
}

constexpr std::array<uint16_t, 48> GenIndices() {
  return GenIndices(std::index_sequence<0, 1, 2, 2, 1, 3>());
}

constexpr std::array<uint16_t, 48> Indices = GenIndices();

void WindowDecorations::_update() noexcept {
  if (!m_dirty)
    return;

  // Must be called within a draw context
  if (!m_ibo) {
    m_ibo = hsh::create_index_buffer(Indices);
    m_tex = hsh::create_texture2d(
        {window_decorations.width, window_decorations.height}, hsh::RGBA8_UNORM,
        1, [](void* data, size_t size) {
          std::memcpy(data, window_decorations_data, size);
        });
  }
  if (!m_vFifo) {
    m_vFifo = hsh::create_vertex_fifo(sizeof(Vert) * 24 * 2);
  }

  const auto w = float(m_extent.w);
  const auto h = float(m_extent.h);
  constexpr int32_t CornerDim = 128;
  constexpr int32_t EdgeOff = 48;
  constexpr float EdgeUvOff = 48 / 256.f;
  constexpr int32_t BottomEdgeOff = 56;
  constexpr float BottomEdgeUvOff = 56 / 256.f;

  auto normalizePos = [=](const hsh::offset2d& offset) {
    return hsh::float2(offset.x * 2 / w - 1.f, -offset.y * 2 / h + 1.f);
  };
  auto normalizeVert = [&](const hsh::offset2d& offset, float uvx, float uvy) {
    return Vert{normalizePos(offset), hsh::float2{uvx, uvy}};
  };

  auto updateCorner = [&](Vert* cverts, Vert* everts, uint8_t corner) {
    hsh::offset2d cornerOff{corner & 0x1u ? int32_t(m_extent.w) - CornerDim : 0,
                            corner & 0x2u ? int32_t(m_extent.h) - CornerDim
                                          : 0};
    float uvxOff = corner & 0x1u ? 0.5f : 0.f;
    float uvyOff = corner & 0x2u ? 0.5f : 0.f;
    cverts[0] = normalizeVert(cornerOff, uvxOff, uvyOff);
    cverts[1] = normalizeVert(cornerOff + hsh::offset2d(0, CornerDim), uvxOff,
                              uvyOff + 0.5f);
    cverts[2] = normalizeVert(cornerOff + hsh::offset2d(CornerDim, 0),
                              uvxOff + 0.5f, uvyOff);
    cverts[3] = normalizeVert(cornerOff + hsh::offset2d(CornerDim, CornerDim),
                              uvxOff + 0.5f, uvyOff + 0.5f);
    everts[0] = normalizeVert(
        hsh::offset2d(
            corner & 0x1u ? int32_t(m_extent.w) - CornerDim : CornerDim,
            corner & 0x2u ? int32_t(m_extent.h) - BottomEdgeOff : EdgeOff),
        0.5f, corner & 0x2u ? 1.f - BottomEdgeUvOff : EdgeUvOff);
    everts[1] = normalizeVert(
        hsh::offset2d(corner & 0x1u ? int32_t(m_extent.w) - EdgeOff : EdgeOff,
                      corner & 0x2u ? int32_t(m_extent.h) - CornerDim
                                    : CornerDim),
        corner & 0x1u ? 1.f - EdgeUvOff : EdgeUvOff, 0.5f);
  };

  auto vData = m_vFifo.map<Vert>(24, [&](Vert* verts) {
    updateCorner(verts + 0, verts + 16, 0x0);
    updateCorner(verts + 4, verts + 18, 0x1);
    updateCorner(verts + 8, verts + 20, 0x2);
    updateCorner(verts + 12, verts + 22, 0x3);
  });
  m_binding.hsh_bind(DecorationPipeline(vData, m_ibo.get(), m_tex.get()));

  m_dirty = false;
}

void WindowDecorations::update(const hsh::extent2d& extent) noexcept {
  m_extent = extent;
  m_dirty = true;
}

void WindowDecorations::draw() noexcept {
  _update();
  m_binding.draw_indexed(0, uint32_t(Indices.size()));
}

void WindowDecorations::shutdown() noexcept {
  m_ibo.reset();
  m_tex.reset();
}

hsh::owner<hsh::index_buffer<uint16_t>> WindowDecorations::m_ibo{};
hsh::owner<hsh::texture2d> WindowDecorations::m_tex{};

} // namespace boo2
