#pragma once

#include <hsh/hsh.h>

struct MyFormat {
  hsh::float3 position;
  hsh::float4 color;
};

struct UniformData {
  hsh::float4x4 xf;
  hsh::float3 lightDir;
  float afloat;
  hsh::aligned_float3x3 dynColor;
  float bfloat;
};

struct Binding {
  hsh::owner<hsh::texture2d> Tex;
  hsh::binding Binding;
  hsh::binding& Bind(hsh::uniform_buffer<UniformData> u, hsh::vertex_buffer<MyFormat> v);
};
