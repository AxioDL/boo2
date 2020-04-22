#pragma once

#include <hsh/hsh.h>

struct MyFormat {
  hsh::float3 position;
  hsh::float3 normal;
};

struct UniformData {
  hsh::float4x4 xf;
  hsh::float3 lightDir;
  float afloat;
  hsh::aligned_float3x3 dynColor;
  float bfloat;
};

struct Binding {
  hsh::dynamic_owner<hsh::uniform_buffer<UniformData>> Uniform;
  hsh::owner<hsh::vertex_buffer<MyFormat>> VBO;
  hsh::owner<hsh::texture2d> Tex;
  hsh::binding Binding;
};

Binding BuildPipeline();
