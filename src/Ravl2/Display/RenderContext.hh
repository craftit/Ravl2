#pragma once

#include <cstdint>
#include <SDL2/SDL.h>
#include "Ravl2/Geometry/ScaleTranslate.hh"

// Forward declare ImGui draw list
struct ImDrawList;

namespace Ravl2::DebugDisplay {

//! Context passed to scene nodes during prepare/render.
//! Contains rendering state needed by image nodes and overlay nodes.
struct RenderContext {
  int framebufferWidth = 0;
  int framebufferHeight = 0;

  // Fields for overlay rendering (2D)
  ImDrawList* imguiDrawList = nullptr;        //!< ImGui draw list for overlays
  SDL_FPoint origin{0.f, 0.f};                 //!< Screen-space origin (top-left of content area)
  ScaleTranslate<float, 2> view2D{};           //!< 2D view transform (scale + translation)
};

} // namespace Ravl2::DebugDisplay
