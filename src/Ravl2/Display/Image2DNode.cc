#include "Ravl2/Display/Image2DNode.hh"
#include <cassert>
#include <cstdint>
#include <vector>

#if defined(RAVL2_WITH_BGFX)
#include <bgfx/bgfx.h>
#endif

namespace Ravl2::DebugDisplay {

void Image2DNode::setFromU8(const uint8_t* src, int w, int h) {
  width = w; height = h; format = Image2DFormat::U8;
  dataU8.assign(src, src + static_cast<size_t>(w)*static_cast<size_t>(h));
  dataF32.clear();
#if defined(RAVL2_WITH_BGFX)
  gpuDirty = true;
#endif
}

void Image2DNode::setFromF32(const float* src, int w, int h) {
  width = w; height = h; format = Image2DFormat::F32;
  dataF32.assign(src, src + static_cast<size_t>(w)*static_cast<size_t>(h));
  dataU8.clear();
  // Update cached min/max for auto normalization
  auto [mn, mx] = finiteMinMax(dataF32.data(), width, height, width * int(sizeof(float)));
  cachedMin = mn; cachedMax = mx;
#if defined(RAVL2_WITH_BGFX)
  gpuDirty = true;
#endif
}

std::pair<float, float> Image2DNode::sample(int x, int y) const noexcept {
  const int idx = y*width + x;
  if (format == Image2DFormat::U8) {
    float v = static_cast<float>(dataU8[static_cast<size_t>(idx)]) / 255.0f;
    return {v, v};
  } else {
    float v = dataF32[static_cast<size_t>(idx)];
    float mn = cachedMin, mx = cachedMax;
    if (norm.policy == NormalizationPolicy::Fixed) {
      mn = norm.minVal; mx = norm.maxVal;
    }
    if (mx <= mn) mx = mn + 1.0f;
    float n = (v - mn) / (mx - mn);
    return {v, n};
  }
}

void Image2DNode::prepare(RenderContext &ctx) {
  (void)ctx;
#if defined(RAVL2_WITH_BGFX)
  // Create or update R8 texture from CPU data
  if (width <= 0 || height <= 0) return;
  const uint16_t tw = static_cast<uint16_t>(width);
  const uint16_t th = static_cast<uint16_t>(height);

  // Recreate texture on size change
  const bool sizeChanged = (texWidth != tw) || (texHeight != th);
  if (sizeChanged && textureHandleIdx != UINT16_MAX) {
    bgfx::TextureHandle thdl{textureHandleIdx};
    if (bgfx::isValid(thdl)) bgfx::destroy(thdl);
    textureHandleIdx = UINT16_MAX;
  }

  if (textureHandleIdx == UINT16_MAX) {
    const uint64_t flags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIN_POINT;
    bgfx::TextureHandle thdl = bgfx::createTexture2D(tw, th, false, 1, bgfx::TextureFormat::R8, flags);
    if (!bgfx::isValid(thdl)) {
      return;
    }
    textureHandleIdx = thdl.idx;
    texWidth = tw; texHeight = th;
    gpuDirty = true;
  }

  if (!gpuDirty) return;

  // Upload data according to format, normalizing f32 to u8 if needed
  bgfx::TextureHandle thdl{textureHandleIdx};
  if (!bgfx::isValid(thdl)) return;

  if (format == Image2DFormat::U8 && !dataU8.empty()) {
    const size_t sz = static_cast<size_t>(width) * static_cast<size_t>(height);
    const bgfx::Memory* mem = bgfx::copy(dataU8.data(), static_cast<uint32_t>(sz));
    bgfx::updateTexture2D(thdl, 0, 0, 0, 0, tw, th, mem);
  } else if (format == Image2DFormat::F32 && !dataF32.empty()) {
    float mn = cachedMin, mx = cachedMax;
    if (norm.policy == NormalizationPolicy::Fixed) {
      mn = norm.minVal; mx = norm.maxVal;
    }
    if (mx <= mn) mx = mn + 1.0f;
    std::vector<uint8_t> tmp(static_cast<size_t>(width) * static_cast<size_t>(height));
    const float scale = 255.0f / (mx - mn);
    for (int i = 0, n = width*height; i < n; ++i) {
      float v = (dataF32[static_cast<size_t>(i)] - mn) * scale;
      if (!std::isfinite(v)) v = 0.0f;
      if (v < 0.0f) v = 0.0f;
      if (v > 255.0f) v = 255.0f;
      tmp[static_cast<size_t>(i)] = static_cast<uint8_t>(v + 0.5f);
    }
    const bgfx::Memory* mem = bgfx::copy(tmp.data(), static_cast<uint32_t>(tmp.size()));
    bgfx::updateTexture2D(thdl, 0, 0, 0, 0, tw, th, mem);
  }
  gpuDirty = false;
#endif
}

void Image2DNode::render(RenderContext &ctx) {
  (void)ctx;
  // Drawing is performed by the higher-level UI (ImGui using bgfx backend) or a renderer pass.
}

} // namespace Ravl2::DebugDisplay
