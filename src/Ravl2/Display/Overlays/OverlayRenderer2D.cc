#include "Ravl2/Display/Overlays/OverlayRenderer2D.hh"

#if defined(RAVL2_WITH_IMGUI)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#pragma GCC diagnostic pop
#endif

namespace Ravl2::DebugDisplay::Overlays {

static inline ImVec2 toScreen(const SDL_FPoint& origin,
                              const Ravl2::ScaleTranslate<float,2>& view,
                              float ix, float iy) noexcept {
  const float sx = view.scaleVector()[0];
  const float sy = view.scaleVector()[1];
  const float tx = view.translation()[0];
  const float ty = view.translation()[1];
  return ImVec2(origin.x + tx + ix * sx, origin.y + ty + iy * sy);
}

void Points2DOverlay::render(ImDrawList* drawList,
                             const SDL_FPoint& origin,
                             const Ravl2::ScaleTranslate<float,2>& view,
                             int imgW,
                             int imgH) noexcept {
  (void)imgW; (void)imgH;
#if defined(RAVL2_WITH_IMGUI)
  if (!drawList) return;
  for (const auto& p : points) {
    // Optionally skip if far outside typical image bounds (cheap guard)
    if (p.x < -4096.f || p.y < -4096.f) continue;
    ImVec2 sp = toScreen(origin, view, p.x, p.y);
    drawList->AddCircleFilled(sp, radius, rgba, 16);
  }
#else
  (void)drawList; (void)origin; (void)view;
#endif
}

void Lines2DOverlay::render(ImDrawList* drawList,
                            const SDL_FPoint& origin,
                            const Ravl2::ScaleTranslate<float,2>& view,
                            int imgW,
                            int imgH) noexcept {
  (void)imgW; (void)imgH;
#if defined(RAVL2_WITH_IMGUI)
  if (!drawList) return;
  if (vertices.size() < 2) return;
  for (size_t i = 1; i < vertices.size(); ++i) {
    const auto& a = vertices[i-1];
    const auto& b = vertices[i];
    ImVec2 sa = toScreen(origin, view, a.x, a.y);
    ImVec2 sb = toScreen(origin, view, b.x, b.y);
    drawList->AddLine(sa, sb, rgba, thickness);
  }
#else
  (void)drawList; (void)origin; (void)view;
#endif
}

} // namespace Ravl2::DebugDisplay::Overlays
