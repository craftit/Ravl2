#pragma once

#include <cstdint>
#include "Ravl2/Types.hh"
#include "Ravl2/Display/Viewport3DNode.hh"

#if defined(RAVL2_WITH_IMGUI)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#pragma GCC diagnostic pop
#endif

namespace Ravl2::DebugDisplay::Render3D
{

  //! Renders a simple XZ grid at y=0 into an ImGui draw list using CPU projection.
  //! Depth is not considered; grid is drawn as an overlay (always visible).
  //! @param cam Orbit camera providing view/projection parameters.
  //! @param rect Viewport rect in screen coordinates (pixels).
  //! @param drawList ImGui draw list to emit line segments into.
  //! @param cellSize Grid cell size in world units.
  //! @param halfCells Number of cells from origin to each side (extent = halfCells * cellSize).
  void drawGridImGui(const OrbitCamera &cam,
                     const Viewport3DNode::Rect &rect,
                     ImDrawList *drawList,
                     float cellSize,
                     int halfCells) noexcept;

}// namespace Ravl2::DebugDisplay::Render3D
