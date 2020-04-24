#pragma once
#include <hsh/hsh.h>

namespace boo2 {

class WindowDecorations {
public:
  struct DoCreate {};
  struct Vert {
    hsh::float2 pos;
    hsh::float2 uv;
  };
  static constexpr int32_t MarginL = 48;
  static constexpr int32_t MarginR = 48;
  static constexpr int32_t MarginT = 48;
  static constexpr int32_t MarginB = 56;

private:
  hsh::dynamic_owner<hsh::vertex_buffer<Vert>> m_vbo;
  static hsh::owner<hsh::index_buffer<uint16_t>> m_ibo;
  static hsh::owner<hsh::texture2d> m_tex;
  hsh::binding m_binding;
  hsh::extent2d m_extent;
  bool m_dirty = true;
  void _update() noexcept;

public:
  void update(const hsh::extent2d& extent) noexcept;
  void draw() noexcept;
  static void shutdown() noexcept;
};

} // namespace boo2
