#include "Ravl2/Display/Ui/Plots.hh"
#include "Ravl2/Display/Channel.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#include <implot.h>
#pragma GCC diagnostic pop

#include <cmath>
#include <vector>
#include <spdlog/spdlog.h>

namespace Ravl2::DebugDisplay::Ui::Plots
{

  void buildPlotsPanel(ChannelRegistry &channels)
  {
    // Ensure the panel is visible the first time by giving it a default position and size.
    ImGuiViewport *vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowViewport(vp->ID);
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + 50.0f, vp->WorkPos.y + 50.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(550.0f, 400.0f), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if(ImGui::Begin("Plots", nullptr, flags)) {

      // Count channels with plot data
      int plotCount = 0;
      channels.forEachChannel([&](ChannelState &ch) {
        if(ch.plotState.has_value() && !ch.plotState->series.empty()) {
          plotCount++;
        }
      });

      if(plotCount == 0) {
        // Show test plot if no real data
        static std::vector<float> xData;
        static std::vector<float> sineData;
        static std::vector<float> cosineData;

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

        ImGui::Text("ImPlot Demo - No plot data yet");
        ImGui::Separator();
        ImGui::TextWrapped("Use ioSave(\"display://channelname:series=data\", data) to add plots.");
        ImGui::Text("Example: ioSave(\"display://myplot:series=data\", myVector);");
        ImGui::Separator();

        if(ImPlot::BeginPlot("Test Plot", ImVec2(-1, -1))) {
          ImPlot::SetupAxes("X Axis", "Y Axis");
          ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 10.0, ImGuiCond_FirstUseEver);
          ImPlot::SetupAxisLimits(ImAxis_Y1, -1.5, 1.5, ImGuiCond_FirstUseEver);
          ImPlot::PlotLine("sin(x)", xData.data(), sineData.data(), static_cast<int>(xData.size()));
          ImPlot::PlotLine("cos(x)", xData.data(), cosineData.data(), static_cast<int>(xData.size()));
          ImPlot::EndPlot();
        }
      } else {
        // Render plots from channel data
        ImGui::Text("Plots: %d channels", plotCount);
        ImGui::Separator();

        int channelIndex = 0;
        channels.forEachChannel([&](ChannelState &ch) {
          if(!ch.plotState.has_value() || ch.plotState->series.empty()) {
            return;
          }

          const PlotState &plotState = ch.plotState.value();

          // Use a fallback label if channel name is empty
          std::string headerLabel;
          if(ch.name.empty()) {
            headerLabel = "Unnamed Plot " + std::to_string(channelIndex) + "##empty_" + std::to_string(reinterpret_cast<uintptr_t>(&ch));
            static bool warnedOnce = false;
            if(!warnedOnce) {
              SPDLOG_WARN("DebugDisplay: Plot channel with empty name detected. Use format: display://channelname:series=...");
              warnedOnce = true;
            }
          } else {
            headerLabel = ch.name;
          }

          channelIndex++;

          if(ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            float plotHeight = plotCount > 1 ? 200.0f : -1.0f;
            std::string plotLabel = headerLabel + "##plot";
            if(ImPlot::BeginPlot(plotLabel.c_str(), ImVec2(-1.0f, plotHeight))) {
              ImPlot::SetupAxes(plotState.xAxisLabel.c_str(), plotState.yAxisLabel.c_str());

              if(plotState.autoFitAxes) {
                ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 1.0, ImGuiCond_FirstUseEver);
                ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0, ImGuiCond_FirstUseEver);
              } else {
                ImPlot::SetupAxisLimits(ImAxis_X1, plotState.xMin, plotState.xMax, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, plotState.yMin, plotState.yMax, ImGuiCond_Always);
              }

              for(const auto &[name, series] : plotState.series) {
                if(series.x.empty() || series.y.empty()) continue;

                const char *label = series.label.empty() ? name.c_str() : series.label.c_str();
                ImPlot::PlotLine(label, series.x.data(), series.y.data(), static_cast<int>(series.x.size()));
              }

              ImPlot::EndPlot();
            }
          }
        });
      }
    }
    ImGui::End();
  }

}// namespace Ravl2::DebugDisplay::Ui::Plots
