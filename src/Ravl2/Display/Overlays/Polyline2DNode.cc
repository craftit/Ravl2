#include "Ravl2/Display/Overlays/Polyline2DNode.hh"
#include "Ravl2/Display/RenderContext.hh"

#if defined(RAVL2_WITH_IMGUI)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#pragma GCC diagnostic pop
#endif

namespace Ravl2::DebugDisplay
{

  // Helper to transform image-space point to screen-space
  static inline ImVec2 toScreen(const SDL_FPoint &origin,
                                const Ravl2::ScaleTranslate<float, 2> &view,
                                float ix, float iy) noexcept
  {
    const float sx = view.scaleVector()[0];
    const float sy = view.scaleVector()[1];
    const float tx = view.translation()[0];
    const float ty = view.translation()[1];
    return ImVec2(origin.x + tx + ix * sx, origin.y + ty + iy * sy);
  }

  void Polyline2DNode::render(RenderContext &ctx)
  {
#if defined(RAVL2_WITH_IMGUI)
    ImDrawList *drawList = ctx.imguiDrawList;
    if(!drawList) return;

    const size_t n = m_vertices.size();
    if(n < 2) return;

    // Draw line segments
    for(size_t i = 1; i < n; ++i) {
      const auto &a = m_vertices[i - 1];
      const auto &b = m_vertices[i];
      ImVec2 sa = toScreen(ctx.origin, ctx.view2D, a.x, a.y);
      ImVec2 sb = toScreen(ctx.origin, ctx.view2D, b.x, b.y);
      drawList->AddLine(sa, sb, m_rgba, m_thickness);
    }

    // Draw closing segment if closed
    if(m_closed && n >= 3) {
      const auto &a = m_vertices.back();
      const auto &b = m_vertices.front();
      ImVec2 sa = toScreen(ctx.origin, ctx.view2D, a.x, a.y);
      ImVec2 sb = toScreen(ctx.origin, ctx.view2D, b.x, b.y);
      drawList->AddLine(sa, sb, m_rgba, m_thickness);
    }
#else
    (void)ctx;
#endif
  }

}// namespace Ravl2::DebugDisplay
