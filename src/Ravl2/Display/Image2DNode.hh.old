#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Ravl2/Display/ISceneNode.hh"
#include "Ravl2/Display/Normalization.hh"
#include "Ravl2/Display/RenderContext.hh"

namespace Ravl2::DebugDisplay {

//! CPU-side image formats supported in Phase 4 MVP.
enum class Image2DFormat {
  U8,   //!< Single-channel uint8_t
  F32   //!< Single-channel float32
};

//! Persistent node for a 2D image. Owns CPU data and GPU texture (future bgfx hookup).
struct Image2DNode : public ISceneNode {
  // Dimensions and format
  int width = 0;
  int height = 0;
  Image2DFormat format = Image2DFormat::U8;

  // CPU storage
  std::vector<uint8_t> dataU8;    //!< If format==U8, size=width*height
  std::vector<float> dataF32;     //!< If format==F32, size=width*height

  // Display/normalization
  NormalizationSettings norm{};

  // Cached min/max for F32 Auto normalization
  float cachedMin = 0.0f;
  float cachedMax = 1.0f;

#if defined(RAVL2_WITH_BGFX)
  // GPU texture resources (bgfx)
  uint16_t texWidth = 0;
  uint16_t texHeight = 0;
  bool gpuDirty = true;            //!< Marked when CPU data or normalization changes
  // Store as uint16_t handle index to avoid including bgfx headers in the header file
  uint16_t textureHandleIdx = UINT16_MAX; //!< bgfx::kInvalidHandle as UINT16_MAX when not created
#endif

  Image2DNode() = default;

  void setFromU8(const uint8_t* src, int w, int h);
  void setFromF32(const float* src, int w, int h);

  // Sample original value at integer pixel (no bounds check); returns original and normalized [0,1]
  std::pair<float, float> sample(int x, int y) const noexcept;

  void prepare(RenderContext &ctx) override;
  void render(RenderContext &ctx) override;
};

} // namespace Ravl2::DebugDisplay
