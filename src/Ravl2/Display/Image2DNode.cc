#include "Ravl2/Display/Image2DNode.hh"
#include <cassert>

namespace Ravl2::DebugDisplay {

void Image2DNode::setFromU8(const uint8_t* src, int w, int h) {
  width = w; height = h; format = Image2DFormat::U8;
  dataU8.assign(src, src + static_cast<size_t>(w)*static_cast<size_t>(h));
  dataF32.clear();
}

void Image2DNode::setFromF32(const float* src, int w, int h) {
  width = w; height = h; format = Image2DFormat::F32;
  dataF32.assign(src, src + static_cast<size_t>(w)*static_cast<size_t>(h));
  dataU8.clear();
  // Update cached min/max for auto normalization
  auto [mn, mx] = finiteMinMax(dataF32.data(), width, height, width * int(sizeof(float)));
  cachedMin = mn; cachedMax = mx;
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
  // Placeholder: when bgfx is wired, upload/update GPU texture here.
}

void Image2DNode::render(RenderContext &ctx) {
  (void)ctx;
  // Placeholder: when bgfx is wired, submit textured quad here.
}

} // namespace Ravl2::DebugDisplay
