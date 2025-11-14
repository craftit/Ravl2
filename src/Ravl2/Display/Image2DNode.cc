#include "Ravl2/Display/Image2DNode.hh"
#include <cassert>
#include <cstdint>
#include <vector>
#include <format>

#if defined(RAVL2_WITH_BGFX)
#include <bgfx/bgfx.h>
#endif

namespace Ravl2::DebugDisplay {

// ============================================================================
// Image2DNode<uint8_t> specialization
// ============================================================================

void Image2DNode<uint8_t>::setData(std::vector<uint8_t> data, int w, int h) {
  pixelData = std::move(data);
  width = w;
  height = h;
#if defined(RAVL2_WITH_BGFX)
  gpuDirty = true;
#endif
}

void Image2DNode<uint8_t>::setData(const uint8_t* data, int w, int h) {
  const size_t sz = static_cast<size_t>(w) * static_cast<size_t>(h);
  pixelData.assign(data, data + sz);
  width = w;
  height = h;
#if defined(RAVL2_WITH_BGFX)
  gpuDirty = true;
#endif
}

PixelQueryResult Image2DNode<uint8_t>::queryPixelInfo(int x, int y) const {
  if (x < 0 || x >= width || y < 0 || y >= height) {
    return {.valid = false, .coordinateText = "", .valueText = "", .extraInfo = std::nullopt};
  }

  const uint8_t value = pixelData[static_cast<size_t>(y * width + x)];

  return {
    .valid = true,
    .coordinateText = std::format("x: {}, y: {}", x, y),
    .valueText = std::format("Value: {}", value),
    .extraInfo = std::nullopt
  };
}

void Image2DNode<uint8_t>::uploadToGPU() {
#if defined(RAVL2_WITH_BGFX)
  if (pixelData.empty()) return;

  bgfx::TextureHandle thdl{textureHandleIdx};
  if (!bgfx::isValid(thdl)) return;

  const size_t sz = static_cast<size_t>(width) * static_cast<size_t>(height);
  // Convert grayscale to RGBA8 by replicating to RGB and setting alpha to 255
  std::vector<uint8_t> rgba(sz * 4);
  for (size_t i = 0; i < sz; ++i) {
    uint8_t v = pixelData[i];
    rgba[i*4 + 0] = v; // R
    rgba[i*4 + 1] = v; // G
    rgba[i*4 + 2] = v; // B
    rgba[i*4 + 3] = 255; // A
  }
  const bgfx::Memory* mem = bgfx::copy(rgba.data(), static_cast<uint32_t>(rgba.size()));
  bgfx::updateTexture2D(thdl, 0, 0, 0, 0, texWidth, texHeight, mem);
#endif
}

std::pair<float, float> Image2DNode<uint8_t>::sample(int x, int y) const noexcept {
  const int idx = y * width + x;
  float v = static_cast<float>(pixelData[static_cast<size_t>(idx)]) / 255.0f;
  return {v, v};
}

// ============================================================================
// Image2DNode<float> specialization
// ============================================================================

void Image2DNode<float>::setData(std::vector<float> data, int w, int h) {
  pixelData = std::move(data);
  width = w;
  height = h;

  // Update cached min/max for auto normalization
  auto [mn, mx] = finiteMinMax(pixelData.data(), width, height, width * int(sizeof(float)));
  cachedMin = mn;
  cachedMax = mx;

#if defined(RAVL2_WITH_BGFX)
  gpuDirty = true;
#endif
}

void Image2DNode<float>::setData(const float* data, int w, int h) {
  const size_t sz = static_cast<size_t>(w) * static_cast<size_t>(h);
  pixelData.assign(data, data + sz);
  width = w;
  height = h;

  // Update cached min/max for auto normalization
  auto [mn, mx] = finiteMinMax(pixelData.data(), width, height, width * int(sizeof(float)));
  cachedMin = mn;
  cachedMax = mx;

#if defined(RAVL2_WITH_BGFX)
  gpuDirty = true;
#endif
}

std::string Image2DNode<float>::formatValue(const float& value) const {
  float mn = cachedMin, mx = cachedMax;
  if (norm.policy == NormalizationPolicy::Fixed) {
    mn = norm.minVal;
    mx = norm.maxVal;
  }
  if (mx <= mn) mx = mn + 1.0f;
  float displayValue = (value - mn) / (mx - mn);
  if (displayValue < 0.0f) displayValue = 0.0f;
  if (displayValue > 1.0f) displayValue = 1.0f;

  return std::format("Raw: {:.3f}, Display: {:.3f}", value, displayValue);
}

std::string Image2DNode<float>::formatExtra(const float& value) const {
  (void)value; // Not used for extra info, just for range
  return std::format("Range: [{:.3f}, {:.3f}]", cachedMin, cachedMax);
}

PixelQueryResult Image2DNode<float>::queryPixelInfo(int x, int y) const {
  if (x < 0 || x >= width || y < 0 || y >= height) {
    return {.valid = false, .coordinateText = "", .valueText = "", .extraInfo = std::nullopt};
  }

  const float value = pixelData[static_cast<size_t>(y * width + x)];

  return {
    .valid = true,
    .coordinateText = std::format("x: {}, y: {}", x, y),
    .valueText = formatValue(value),
    .extraInfo = formatExtra(value)
  };
}

void Image2DNode<float>::uploadToGPU() {
#if defined(RAVL2_WITH_BGFX)
  if (pixelData.empty()) return;

  bgfx::TextureHandle thdl{textureHandleIdx};
  if (!bgfx::isValid(thdl)) return;

  float mn = cachedMin, mx = cachedMax;
  if (norm.policy == NormalizationPolicy::Fixed) {
    mn = norm.minVal;
    mx = norm.maxVal;
  }
  if (mx <= mn) mx = mn + 1.0f;

  const size_t sz = static_cast<size_t>(width) * static_cast<size_t>(height);
  std::vector<uint8_t> rgba(sz * 4);
  const float scale = 255.0f / (mx - mn);

  for (size_t i = 0; i < sz; ++i) {
    float v = (pixelData[i] - mn) * scale;
    if (!std::isfinite(v)) v = 0.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 255.0f) v = 255.0f;
    uint8_t u8v = static_cast<uint8_t>(v + 0.5f);
    rgba[i*4 + 0] = u8v; // R
    rgba[i*4 + 1] = u8v; // G
    rgba[i*4 + 2] = u8v; // B
    rgba[i*4 + 3] = 255; // A
  }

  const bgfx::Memory* mem = bgfx::copy(rgba.data(), static_cast<uint32_t>(rgba.size()));
  bgfx::updateTexture2D(thdl, 0, 0, 0, 0, texWidth, texHeight, mem);
#endif
}

std::pair<float, float> Image2DNode<float>::sample(int x, int y) const noexcept {
  const int idx = y * width + x;
  float v = pixelData[static_cast<size_t>(idx)];
  float mn = cachedMin, mx = cachedMax;
  if (norm.policy == NormalizationPolicy::Fixed) {
    mn = norm.minVal;
    mx = norm.maxVal;
  }
  if (mx <= mn) mx = mn + 1.0f;
  float n = (v - mn) / (mx - mn);
  if (n < 0.0f) n = 0.0f;
  if (n > 1.0f) n = 1.0f;
  return {v, n};
}

// ============================================================================
// Image2DNode<PixelRGB8> specialization
// ============================================================================

void Image2DNode<PixelRGB8>::setData(std::vector<PixelRGB8> data, int w, int h) {
  pixelData = std::move(data);
  width = w;
  height = h;
#if defined(RAVL2_WITH_BGFX)
  gpuDirty = true;
#endif
}

void Image2DNode<PixelRGB8>::setData(const PixelRGB8* data, int w, int h) {
  const size_t sz = static_cast<size_t>(w) * static_cast<size_t>(h);
  pixelData.assign(data, data + sz);
  width = w;
  height = h;
#if defined(RAVL2_WITH_BGFX)
  gpuDirty = true;
#endif
}

std::string Image2DNode<PixelRGB8>::formatValue(const PixelRGB8& value) const {
  return std::format("R:{} G:{} B:{}",
                     static_cast<int>(value.template get<ImageChannel::Red>()),
                     static_cast<int>(value.template get<ImageChannel::Green>()),
                     static_cast<int>(value.template get<ImageChannel::Blue>()));
}

PixelQueryResult Image2DNode<PixelRGB8>::queryPixelInfo(int x, int y) const {
  if (x < 0 || x >= width || y < 0 || y >= height) {
    return {.valid = false, .coordinateText = "", .valueText = "", .extraInfo = std::nullopt};
  }

  const PixelRGB8& value = pixelData[static_cast<size_t>(y * width + x)];

  return {
    .valid = true,
    .coordinateText = std::format("x: {}, y: {}", x, y),
    .valueText = formatValue(value),
    .extraInfo = std::nullopt
  };
}

void Image2DNode<PixelRGB8>::uploadToGPU() {
#if defined(RAVL2_WITH_BGFX)
  if (pixelData.empty()) return;

  bgfx::TextureHandle thdl{textureHandleIdx};
  if (!bgfx::isValid(thdl)) return;

  const size_t sz = static_cast<size_t>(width) * static_cast<size_t>(height);
  // Convert RGB to RGBA8 by adding alpha channel (255 = opaque)
  std::vector<uint8_t> rgba(sz * 4);
  for (size_t i = 0; i < sz; ++i) {
    const PixelRGB8& pixel = pixelData[i];
    rgba[i*4 + 0] = pixel.template get<ImageChannel::Red>();
    rgba[i*4 + 1] = pixel.template get<ImageChannel::Green>();
    rgba[i*4 + 2] = pixel.template get<ImageChannel::Blue>();
    rgba[i*4 + 3] = 255; // A
  }
  const bgfx::Memory* mem = bgfx::copy(rgba.data(), static_cast<uint32_t>(rgba.size()));
  bgfx::updateTexture2D(thdl, 0, 0, 0, 0, texWidth, texHeight, mem);
#endif
}

} // namespace Ravl2::DebugDisplay
