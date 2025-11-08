#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace Ravl2::DebugDisplay {

//! Normalization policies for displaying float images.
enum class NormalizationPolicy {
  Auto,        //!< scale per-image min/max to [0,1]
  Fixed,       //!< user-provided [min,max]
  Percentile   //!< clamp to [pLow,pHigh] percentiles then scale
};

struct NormalizationSettings {
  NormalizationPolicy policy = NormalizationPolicy::Auto;
  float minVal = 0.0f;   //!< Used for Fixed
  float maxVal = 1.0f;   //!< Used for Fixed
  float lowPct = 1.0f;   //!< Used for Percentile (e.g., 1)
  float highPct = 99.0f; //!< Used for Percentile (e.g., 99)
};

//! Compute min/max ignoring NaN/Inf; returns {min,max}. If no finite values, returns {0,1}.
inline std::pair<float,float> finiteMinMax(const float* data, int width, int height, int stride) noexcept {
  float mn = std::numeric_limits<float>::infinity();
  float mx = -std::numeric_limits<float>::infinity();
  for (int y=0; y<height; ++y) {
    const float* row = reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(data) + size_t(y)*size_t(stride));
    for (int x=0; x<width; ++x) {
      float v = row[x];
      if (!std::isfinite(v)) continue;
      if (v < mn) mn = v;
      if (v > mx) mx = v;
    }
  }
  if (!std::isfinite(mn) || !std::isfinite(mx) || mn == mx) {
    return {0.0f, 1.0f};
  }
  return {mn, mx};
}

//! Compute percentiles (approximate) by sampling values into a vector and nth_element.
inline std::pair<float,float> percentiles(const float* data, int width, int height, int stride, float lowPct, float highPct) {
  std::vector<float> vals;
  vals.reserve(size_t(width)*size_t(height));
  for (int y=0; y<height; ++y) {
    const float* row = reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(data) + size_t(y)*size_t(stride));
    for (int x=0; x<width; ++x) {
      float v = row[x];
      if (std::isfinite(v)) vals.push_back(v);
    }
  }
  if (vals.empty()) return {0.0f,1.0f};
  auto clampPct = [](float p){ return std::clamp(p, 0.0f, 100.0f); };
  lowPct = clampPct(lowPct);
  highPct = clampPct(highPct);
  if (highPct < lowPct) std::swap(lowPct, highPct);
  const size_t n = vals.size();
  const double nMinus1 = static_cast<double>(n > 0 ? (n - 1) : 0);
  const size_t iLow = static_cast<size_t>(std::floor((static_cast<double>(lowPct)/100.0) * nMinus1));
  const size_t iHigh = static_cast<size_t>(std::floor((static_cast<double>(highPct)/100.0) * nMinus1));
  std::nth_element(vals.begin(), vals.begin() + static_cast<std::ptrdiff_t>(iLow), vals.end());
  float vLow = vals[static_cast<size_t>(iLow)];
  std::nth_element(vals.begin(), vals.begin() + static_cast<std::ptrdiff_t>(iHigh), vals.end());
  float vHigh = vals[static_cast<size_t>(iHigh)];
  if (vHigh == vLow) vHigh = vLow + 1.0f;
  return {vLow, vHigh};
}

//! Normalize a float row to uint8 [0,255] with given min/max.
inline void normalizeRowToU8(const float* src, uint8_t* dst, int width, float mn, float mx) noexcept {
  const float scale = 255.0f / (mx - mn);
  for (int x=0; x<width; ++x) {
    float v = src[x];
    float t = (v - mn) * scale;
    if (!std::isfinite(t)) t = 0.0f;
    t = std::clamp(t, 0.0f, 255.0f);
    dst[x] = static_cast<uint8_t>(t + 0.5f);
  }
}

} // namespace Ravl2::DebugDisplay
