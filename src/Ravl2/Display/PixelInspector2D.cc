#include "Ravl2/Display/PixelInspector2D.hh"

#include <utility>

#include "Ravl2/Display/Image2DNode.hh"
#include "Ravl2/Display/Normalization.hh"

namespace Ravl2::DebugDisplay {

std::optional<PixelInfo2D> PixelInspector2D::inspect(
    int mouseX, int mouseY,
    const std::unordered_map<std::string, SDL_FRect>& lastRects,
    const std::unordered_map<std::string, SDL_FPoint>& imageOrigins,
    ChannelRegistry& channels) const noexcept
{
  // Find first channel whose last drawn rect contains the mouse
  std::string under;
  SDL_FRect rect{};
  const float fx = static_cast<float>(mouseX);
  const float fy = static_cast<float>(mouseY);
  for (const auto& kv : lastRects) {
    const auto& r = kv.second;
    if (fx >= r.x && fx < r.x + r.w && fy >= r.y && fy < r.y + r.h) {
      under = kv.first;
      rect = r;
      break;
    }
  }
  if (under.empty()) return std::nullopt;

  auto& ch = channels.getOrCreateChannel(under);
  if (!ch.baseImage2D) return std::nullopt;
  auto* node = static_cast<Image2DNode*>(ch.baseImage2D.get());
  const int w = node->width;
  const int h = node->height;
  if (w <= 0 || h <= 0) return std::nullopt;

  // Map mouse to image pixel using image origin + view2D transform parameters
  const float sx = ch.view2D.scaleVector()[0];
  const float sy = ch.view2D.scaleVector()[1];
  const float tx = ch.view2D.translation()[0];
  const float ty = ch.view2D.translation()[1];
  SDL_FPoint origin{0.f, 0.f};
  if (auto it = imageOrigins.find(under); it != imageOrigins.end()) {
    origin = it->second;
  }
  const float denomX = (sx != 0.0f) ? sx : 1.0f;
  const float denomY = (sy != 0.0f) ? sy : 1.0f;
  const int ix = static_cast<int>((fx - (origin.x + tx)) / denomX);
  const int iy = static_cast<int>((fy - (origin.y + ty)) / denomY);
  if (ix < 0 || iy < 0 || ix >= w || iy >= h) return std::nullopt;

  PixelInfo2D out{};
  out.channel = under;
  out.ix = ix;
  out.iy = iy;

  const int idx = iy * w + ix;
  if (node->format == Image2DFormat::U8 && !node->dataU8.empty()) {
    const float v = static_cast<float>(node->dataU8[static_cast<size_t>(idx)]) / 255.0f;
    out.raw = v;
    out.disp = v;
    out.minUsed = 0.0f; out.maxUsed = 1.0f;
  } else if (node->format == Image2DFormat::F32 && !node->dataF32.empty()) {
    const float v = node->dataF32[static_cast<size_t>(idx)];
    out.raw = v;
    float mn = node->cachedMin;
    float mx = node->cachedMax;
    switch (ch.norm.policy) {
      case NormalizationPolicy::Auto:
        break;
      case NormalizationPolicy::Fixed:
        mn = ch.norm.minVal; mx = ch.norm.maxVal;
        break;
      case NormalizationPolicy::Percentile: {
        auto mm = percentiles(node->dataF32.data(), w, h, w * int(sizeof(float)), ch.norm.lowPct, ch.norm.highPct);
        mn = mm.first; mx = mm.second;
        break;
      }
    }
    if (mx <= mn) mx = mn + 1.0f;
    out.minUsed = mn; out.maxUsed = mx;
    float disp = (v - mn) / (mx - mn);
    if (disp < 0.0f) disp = 0.0f;
    if (disp > 1.0f) disp = 1.0f;
    out.disp = disp;
  } else {
    return std::nullopt;
  }

  (void)rect; // kept for potential future use (sub-rect mapping)
  return out;
}

} // namespace Ravl2::DebugDisplay
