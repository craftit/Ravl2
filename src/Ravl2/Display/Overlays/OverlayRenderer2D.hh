#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <SDL2/SDL.h>

#include "Ravl2/Geometry/ScaleTranslate.hh"

// Forward declare ImGui draw list to avoid heavy includes in headers
struct ImDrawList;

namespace Ravl2::DebugDisplay::Overlays {

//! Interface for 2D overlays rendered over an image in a channel window.
//! Coordinate system: image space (0..width-1, 0..height-1) mapped to screen via
//!   screen = origin + translation + (image * scale)
//! where origin is the channel window content top-left (screen space),
//! translation and scale come from ChannelState::view2D.
struct OverlayRenderer2D {
  virtual ~OverlayRenderer2D() = default;

  //! Render overlay primitives into the provided ImGui draw list.
  //! @param drawList ImGui window draw list (never null when called)
  //! @param origin Screen-space origin of the image content (top-left of content region)
  //! @param view Current 2D view transform (scale, translation)
  //! @param imgW Image width in pixels
  //! @param imgH Image height in pixels
  virtual void render(ImDrawList* drawList,
                      const SDL_FPoint& origin,
                      const Ravl2::ScaleTranslate<float,2>& view,
                      int imgW,
                      int imgH) noexcept = 0;
};

//! Simple overlay: a set of points in image coordinates.
struct Points2DOverlay final : public OverlayRenderer2D {
  std::vector<SDL_FPoint> points; // image-space points
  uint32_t rgba = 0xff00ffffu;    // default magenta, ABGR in ImGui expects ImU32 packed RGBA
  float radius = 2.0f;            // radius in screen pixels

  void render(ImDrawList* drawList,
              const SDL_FPoint& origin,
              const Ravl2::ScaleTranslate<float,2>& view,
              int imgW,
              int imgH) noexcept override;
};

//! Simple overlay: polyline connecting given image-space vertices.
struct Lines2DOverlay final : public OverlayRenderer2D {
  std::vector<SDL_FPoint> vertices; // image-space
  uint32_t rgba = 0xff00ff00u;      // default green
  float thickness = 1.0f;           // line thickness in screen px

  void render(ImDrawList* drawList,
              const SDL_FPoint& origin,
              const Ravl2::ScaleTranslate<float,2>& view,
              int imgW,
              int imgH) noexcept override;
};

} // namespace Ravl2::DebugDisplay::Overlays
