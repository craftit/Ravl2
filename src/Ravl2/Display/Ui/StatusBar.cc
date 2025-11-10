#include "Ravl2/Display/Ui/StatusBar.hh"

#if defined(RAVL2_WITH_IMGUI)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#pragma GCC diagnostic pop
#endif

#include "Ravl2/Display/PixelInspector2D.hh"

namespace Ravl2::DebugDisplay::Ui::StatusBar {

void build(const std::unordered_map<std::string, SDL_FRect>& lastRects,
           ChannelRegistry& channels)
{
#if defined(RAVL2_WITH_IMGUI)
  ImGuiViewport* vp = ImGui::GetMainViewport();
  // Place at bottom on first use; remain dockable and movable later by the user.
  ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y), ImGuiCond_FirstUseEver, ImVec2(0.0f, 1.0f));
  ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, 0.0f), ImGuiCond_FirstUseEver);
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
  if (ImGui::Begin("Status", nullptr, flags)) {
    int mx = 0, my = 0;
    // Use ImGui mouse position to match ImGui screen-space rects recorded in ChannelWindows
    ImVec2 mp = ImGui::GetMousePos();
    mx = static_cast<int>(mp.x);
    my = static_cast<int>(mp.y);
    PixelInspector2D inspector;
    if (auto info = inspector.inspect(mx, my, lastRects, channels)) {
      ImGui::Text("%s  x=%d y=%d  orig=%.6g  disp=%.4f  [range %.6g..%.6g]",
                  info->channel.c_str(), info->ix, info->iy,
                  static_cast<double>(info->raw), static_cast<double>(info->disp),
                  static_cast<double>(info->minUsed), static_cast<double>(info->maxUsed));
    } else {
      ImGui::TextUnformatted("Hover an image to inspect pixels…");
    }
  }
  ImGui::End();
#else
  (void)lastRects; (void)channels;
#endif
}

} // namespace Ravl2::DebugDisplay::Ui::StatusBar
