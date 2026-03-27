#include "Ravl2/Display/Image2DNodeBase.hh"

#if defined(RAVL2_WITH_BGFX)
#include <bgfx/bgfx.h>
#endif

namespace Ravl2::DebugDisplay
{

  Image2DNodeBase::~Image2DNodeBase()
  {
#if defined(RAVL2_WITH_BGFX)
    // Clean up GPU texture if valid
    if(textureHandleIdx != UINT16_MAX) {
      bgfx::TextureHandle thdl {textureHandleIdx};
      if(bgfx::isValid(thdl)) {
        bgfx::destroy(thdl);
      }
    }
#endif
  }

  void Image2DNodeBase::prepare(RenderContext &ctx)
  {
    (void)ctx;
#if defined(RAVL2_WITH_BGFX)
    // Create or update RGBA8 texture from CPU data
    if(width <= 0 || height <= 0) return;

    const uint16_t tw = static_cast<uint16_t>(width);
    const uint16_t th = static_cast<uint16_t>(height);

    // Recreate texture on size change
    const bool sizeChanged = (texWidth != tw) || (texHeight != th);
    if(sizeChanged && textureHandleIdx != UINT16_MAX) {
      bgfx::TextureHandle thdl {textureHandleIdx};
      if(bgfx::isValid(thdl)) bgfx::destroy(thdl);
      textureHandleIdx = UINT16_MAX;
    }

    if(textureHandleIdx == UINT16_MAX) {
      const uint64_t flags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIN_POINT;
      bgfx::TextureHandle thdl = bgfx::createTexture2D(tw, th, false, 1,
                                                       bgfx::TextureFormat::RGBA8, flags);
      if(!bgfx::isValid(thdl)) {
        return;
      }
      textureHandleIdx = thdl.idx;
      texWidth = tw;
      texHeight = th;
      gpuDirty = true;
    }

    if(!gpuDirty) return;

    // Call derived class to upload type-specific data
    uploadToGPU();
    gpuDirty = false;
#endif
  }

  void Image2DNodeBase::render(RenderContext &ctx)
  {
    (void)ctx;
    // Drawing is performed by the higher-level UI (ImGui using bgfx backend)
  }

}// namespace Ravl2::DebugDisplay
