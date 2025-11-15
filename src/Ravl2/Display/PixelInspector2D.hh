#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include <SDL2/SDL.h>

#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/ISceneNode.hh"

namespace Ravl2::DebugDisplay {

//! Result of a 2D pixel inspection at the current mouse position.
//! Legacy struct - kept for backward compatibility with existing code.
struct PixelInfo2D {
  std::string channel;
  int ix = -1;   // image-space x (column)
  int iy = -1;   // image-space y (row)
  float raw = 0; // original value; for U8 this is normalized to [0,1]
  float disp = 0; // display-normalized value in [0,1]
  float minUsed = 0; // min used for normalization (for F32)
  float maxUsed = 1; // max used for normalization (for F32)
};

//! Extended result with both legacy and new query result
struct PixelInspectResult {
  std::string channel;
  PixelQueryResult queryResult;  //!< New formatted query result from ISceneNode
  std::optional<PixelInfo2D> legacyInfo;  //!< Legacy numeric info (optional)
};

//! Computes pixel information under the mouse cursor using last drawn rects
//! and channel registry. Uses ISceneNode query interface for formatted output.
class PixelInspector2D {
public:
  PixelInspector2D() = default;

  //! New interface using PixelQueryResult from ISceneNode
  std::optional<PixelInspectResult> inspectWithQuery(
      int mouseX, int mouseY,
      const std::unordered_map<std::string, SDL_FRect>& lastRects,
      const std::unordered_map<std::string, SDL_FPoint>& imageOrigins,
      ChannelRegistry& channels) const noexcept;

  //! Legacy interface - kept for backward compatibility
  std::optional<PixelInfo2D> inspect(int mouseX, int mouseY,
                                     const std::unordered_map<std::string, SDL_FRect>& lastRects,
                                     const std::unordered_map<std::string, SDL_FPoint>& imageOrigins,
                                     ChannelRegistry& channels) const noexcept;
};

} // namespace Ravl2::DebugDisplay
