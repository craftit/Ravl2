#pragma once

#include <cstdint>
#include "Ravl2/Display/ISceneNode.hh"
#include "Ravl2/Display/RenderContext.hh"

namespace Ravl2::DebugDisplay
{

  //! Abstract base class for 2D image nodes with common GPU texture management.
  //! Derived classes implement type-specific data storage and GPU upload.
  class Image2DNodeBase : public ISceneNode
  {
  public:
    // Common properties
    int width = 0;
    int height = 0;

#if defined(RAVL2_WITH_BGFX)
    // GPU texture resources (bgfx)
    uint16_t texWidth = 0;
    uint16_t texHeight = 0;
    bool gpuDirty = true;//!< Marked when CPU data changes
    // Store as uint16_t handle index to avoid including bgfx headers
    uint16_t textureHandleIdx = UINT16_MAX;//!< bgfx::kInvalidHandle as UINT16_MAX
#endif
    virtual ~Image2DNodeBase();

    //! Prepare node for rendering (handles GPU texture creation/upload)
    void prepare(RenderContext &ctx) final;

    //! Render node (currently no-op, rendering handled by ImGui)
    void render(RenderContext &ctx) final;

    //! All image nodes support pixel queries
    bool supportsPixelQuery() const final { return true; }

    //! Query pixel information - must be implemented by derived classes
    PixelQueryResult queryPixelInfo(int x, int y) const override = 0;

    //! Upload type-specific data to GPU - must be implemented by derived classes
    virtual void uploadToGPU() = 0;

    // Accessors
    int getWidth() const { return width; }
    int getHeight() const { return height; }

#if defined(RAVL2_WITH_BGFX)
    uint16_t getTextureHandle() const { return textureHandleIdx; }
#endif
  };

}// namespace Ravl2::DebugDisplay
