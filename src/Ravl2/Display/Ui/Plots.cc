#include "Ravl2/Display/Ui/Plots.hh"

#if defined(RAVL2_WITH_IMGUI)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#pragma GCC diagnostic pop
#endif

namespace Ravl2::DebugDisplay::Ui::Plots {

void buildPlotsPanel()
{
#if defined(RAVL2_WITH_IMGUI)
  // Ensure the panel is visible the first time by giving it a default position and size.
  ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowViewport(vp->ID);
  // Note: SetNextWindowPos/Size accept only a single ImGuiCond flag.
  // Use FirstUseEver so users can move/dock it afterward without it snapping back.
  ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + 50.0f, vp->WorkPos.y + 50.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(420.0f, 260.0f), ImGuiCond_FirstUseEver);

  ImGuiWindowFlags flags = ImGuiWindowFlags_None;
  if (ImGui::Begin("Plots", nullptr, flags)) {
    ImGui::TextUnformatted("Plots panel placeholder");
    ImGui::Separator();
    ImGui::TextUnformatted("ImPlot integration hook: Phase 7 will populate this panel.");
    ImGui::TextUnformatted("- Add series management and plot widgets here.");
  }
  ImGui::End();
#endif
}

} // namespace Ravl2::DebugDisplay::Ui::Plots
