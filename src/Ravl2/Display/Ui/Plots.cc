#include "Ravl2/Display/Ui/Plots.hh"

#if defined(RAVL2_WITH_IMGUI)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#include <implot.h>
#pragma GCC diagnostic pop
#endif

#include <cmath>
#include <vector>

namespace Ravl2::DebugDisplay::Ui::Plots
{

  void buildPlotsPanel()
  {
#if defined(RAVL2_WITH_IMGUI)
    // Ensure the panel is visible the first time by giving it a default position and size.
    ImGuiViewport *vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowViewport(vp->ID);
    // Note: SetNextWindowPos/Size accept only a single ImGuiCond flag.
    // Use FirstUseEver so users can move/dock it afterward without it snapping back.
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + 50.0f, vp->WorkPos.y + 50.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(550.0f, 350.0f), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if(ImGui::Begin("Plots", nullptr, flags)) {
      // Generate test data for sine and cosine waves
      static std::vector<float> xData;
      static std::vector<float> sineData;
      static std::vector<float> cosineData;

      // Initialize test data on first call
      if(xData.empty()) {
        constexpr int numPoints = 200;
        xData.reserve(numPoints);
        sineData.reserve(numPoints);
        cosineData.reserve(numPoints);

        for(int i = 0; i < numPoints; ++i) {
          float x = static_cast<float>(i) * 0.05f;
          xData.push_back(x);
          sineData.push_back(std::sin(x));
          cosineData.push_back(std::cos(x));
        }
      }

      ImGui::Text("ImPlot Test - Phase 7a MVP");
      ImGui::Separator();

      // Create a plot with ImPlot
      if(ImPlot::BeginPlot("Test Plot", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("X Axis", "Y Axis");
        ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 10.0, ImGuiCond_FirstUseEver);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -1.5, 1.5, ImGuiCond_FirstUseEver);

        // Plot sine and cosine series
        ImPlot::PlotLine("sin(x)", xData.data(), sineData.data(), static_cast<int>(xData.size()));
        ImPlot::PlotLine("cos(x)", xData.data(), cosineData.data(), static_cast<int>(xData.size()));

        ImPlot::EndPlot();
      }
    }
    ImGui::End();
#endif
  }

}// namespace Ravl2::DebugDisplay::Ui::Plots
