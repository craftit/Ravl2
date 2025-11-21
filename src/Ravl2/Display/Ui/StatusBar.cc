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

namespace Ravl2::DebugDisplay::Ui::StatusBar
{

  void build(const std::unordered_map<std::string, SDL_FRect> &lastRects,
             const std::unordered_map<std::string, SDL_FPoint> &imageOrigins,
             ChannelRegistry &channels)
  {
    (void)lastRects;
    (void)imageOrigins;
    (void)channels;
#if defined(RAVL2_WITH_IMGUI)
    ImGuiViewport *vp = ImGui::GetMainViewport();
    // Always place at the bottom of the main viewport when not docked.
    // If the window is docked, ImGui will ignore this position.
    ImGui::SetNextWindowViewport(vp->ID);
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, 0.0f), ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
    if(ImGui::Begin("Status", nullptr, flags)) {
      // Use ImGui mouse position to match ImGui screen-space rects recorded in ChannelWindows
      // ImVec2 mp = ImGui::GetMousePos();
      // We should put the extra
      ImGui::TextUnformatted("Hover an image to inspect pixels… NOT IMPLEMENTED");
    }
    ImGui::End();
#endif
  }

}// namespace Ravl2::DebugDisplay::Ui::StatusBar
