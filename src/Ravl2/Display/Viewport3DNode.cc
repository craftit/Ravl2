#include "Ravl2/Display/Viewport3DNode.hh"
#include "Ravl2/Display/RenderContext.hh"

namespace Ravl2::DebugDisplay
{

  void Viewport3DNode::setViewportRect(int x, int y, int w, int h) noexcept
  {
    rect = {x, y, w, h};
    if(w > 0 && h > 0) {
      camera.aspect = static_cast<float>(w) / static_cast<float>(h);
    }
  }

  void Viewport3DNode::prepare(RenderContext &)
  {
    // No resources to prepare in scaffolding stage.
  }

  void Viewport3DNode::render(RenderContext &)
  {
    // No rendering in scaffolding stage.
  }

}// namespace Ravl2::DebugDisplay
