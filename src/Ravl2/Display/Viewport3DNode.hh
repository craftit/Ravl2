#pragma once

#include <cstdint>
#include <optional>
#include "Ravl2/Display/ISceneNode.hh"
#include "Ravl2/Display/OrbitCamera.hh"

namespace Ravl2::DebugDisplay
{

  //! Viewport3D node that manages a 3D camera and view/rect per channel.
  //! @details Owns no GPU resources yet in 6a scaffolding. Will manage bgfx view state in later steps.
  //! @threadsafe No. GUI thread only.
  struct Viewport3DNode final : public ISceneNode {
    // View rectangle in framebuffer coordinates
    struct Rect {
      int x = 0, y = 0, w = 0, h = 0;
    };

    // Assigned bgfx view ID (optional until rendering is wired)
    std::optional<uint16_t> viewId;

    // Orbit camera state
    OrbitCamera camera;

    // Current viewport rect
    Rect rect = {};

    // Grid settings (XZ plane), used by grid renderer in 6a later step
    float gridCellSize = 0.5f;// world units
    int gridHalfCells = 20;   // extent in cells from origin

    // Test scaffolding: last applied point cloud size (CPU-side only)
    std::optional<size_t> lastPointCount;

    // Update viewport; also updates camera aspect if size is positive
    void setViewportRect(int x, int y, int w, int h) noexcept;

    // ISceneNode
    void prepare(RenderContext &) override;
    void render(RenderContext &) override;
  };

}// namespace Ravl2::DebugDisplay
