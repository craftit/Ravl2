#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include <SDL2/SDL.h>

#include "Ravl2/Display/Channel.hh"

namespace Ravl2::DebugDisplay {

//! Result of a 2D pixel inspection at the current mouse position.
struct PixelInfo2D {
  std::string channel;
  int ix = -1;   // image-space x (column)
  int iy = -1;   // image-space y (row)
  float raw = 0; // original value; for U8 this is normalized to [0,1]
  float disp = 0; // display-normalized value in [0,1]
  float minUsed = 0; // min used for normalization (for F32)
  float maxUsed = 1; // max used for normalization (for F32)
};

//! Computes pixel information under the mouse cursor using last drawn rects
//! and channel registry. Consults normalization settings to compute display value.
class PixelInspector2D {
public:
  PixelInspector2D() = default;

  std::optional<PixelInfo2D> inspect(int mouseX, int mouseY,
                                     const std::unordered_map<std::string, SDL_FRect>& lastRects,
                                     const std::unordered_map<std::string, SDL_FPoint>& imageOrigins,
                                     ChannelRegistry& channels) const noexcept;
};

} // namespace Ravl2::DebugDisplay
