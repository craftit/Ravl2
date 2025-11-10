#include "Ravl2/Display/Ui/ControlsPanel.hh"

#include <algorithm>
#include <vector>
#include <spdlog/spdlog.h>

#if defined(RAVL2_WITH_IMGUI)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#pragma GCC diagnostic pop
#endif

#include "Ravl2/Display/Normalization.hh"
#include "Ravl2/Display/Commands/SetNormalization2D.hh"
#include "Ravl2/Display/Image2DNode.hh"

namespace Ravl2::DebugDisplay::Ui {

void buildControlsPanel(float fbw, float fbh,
                        ChannelRegistry& channels,
                        const std::function<void(std::shared_ptr<IRenderCommand>)>& enqueueCmd,
                        std::atomic_bool& invalidated,
                        float zoomMin,
                        float zoomMax)
{
#if defined(RAVL2_WITH_IMGUI)
  // Collect channel names for selection
  static int selectedChannel = 0;
  std::vector<std::string> chNames;
  channels.forEachChannel([&](ChannelState &ch){ chNames.push_back(ch.name); });
  if (selectedChannel >= static_cast<int>(chNames.size())) {
    selectedChannel = chNames.empty() ? 0 : (static_cast<int>(chNames.size()) - 1);
  }

  // Channel selector
  if (!chNames.empty()) {
    const char* current = chNames[static_cast<size_t>(selectedChannel)].c_str();
    if (ImGui::BeginCombo("Channel", current)) {
      for (int i = 0; i < static_cast<int>(chNames.size()); ++i) {
        bool isSelected = (selectedChannel == i);
        if (ImGui::Selectable(chNames[static_cast<size_t>(i)].c_str(), isSelected)) {
          selectedChannel = i;
        }
        if (isSelected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
  } else {
    ImGui::TextUnformatted("No channels");
  }

  // Fetch the selected channel state for editing
  if (!chNames.empty()) {
    const std::string& selName = chNames[static_cast<size_t>(selectedChannel)];
    auto &ch = channels.getOrCreateChannel(selName);

    ImGui::SeparatorText("View");
    // Uniform scale control mapped to both axes
    float sx = ch.view2D.scaleVector()[0];
    float sy = ch.view2D.scaleVector()[1];
    float scaleUniform = (sx + sy) * 0.5f;
    if (ImGui::SliderFloat("Scale", &scaleUniform, zoomMin, std::min(10.0f, zoomMax), "%.3f", ImGuiSliderFlags_Logarithmic)) {
      auto v = ch.view2D.scaleVector();
      v[0] = scaleUniform; v[1] = scaleUniform;
      ch.view2D.scale(v);
      invalidated.store(true, std::memory_order_release);
    }
    auto t = ch.view2D.translation();
    float tx = t[0];
    float ty = t[1];
    if (ImGui::DragFloat("Translate X", &tx, 1.0f)) { t[0] = tx; ch.view2D.translate(t); invalidated.store(true, std::memory_order_release); }
    if (ImGui::DragFloat("Translate Y", &ty, 1.0f)) { t[1] = ty; ch.view2D.translate(t); invalidated.store(true, std::memory_order_release); }

    ImGui::SameLine();
    if (ImGui::Button("Reset View")) {
      ch.view2D = ScaleTranslate<float,2>::identity();
      invalidated.store(true, std::memory_order_release);
    }
    ImGui::SameLine();
    if (ImGui::Button("Fit To Window")) {
      int imgW = 0, imgH = 0;
      if (ch.baseImage2D) {
        if (auto *node = static_cast<Image2DNode*>(ch.baseImage2D.get())) {
          imgW = node->width; imgH = node->height;
        }
      }
      if (imgW > 0 && imgH > 0) {
        const float fbwf = fbw;
        const float fbhf = fbh;
        const float sxFit = fbwf / static_cast<float>(imgW);
        const float syFit = fbhf / static_cast<float>(imgH);
        const float sFit = std::min(sxFit, syFit);
        auto v = ch.view2D.scaleVector(); v[0] = sFit; v[1] = sFit; ch.view2D.scale(v);
        auto tr = ch.view2D.translation();
        tr[0] = (fbwf - static_cast<float>(imgW) * sFit) * 0.5f;
        tr[1] = (fbhf - static_cast<float>(imgH) * sFit) * 0.5f;
        ch.view2D.translate(tr);
        invalidated.store(true, std::memory_order_release);
      }
    }

    ImGui::SeparatorText("Normalization");
    NormalizationSettings ns = ch.norm; // edit copy to avoid partial writes
    int pol = 0;
    switch (ns.policy) {
      case NormalizationPolicy::Auto: pol = 0; break;
      case NormalizationPolicy::Fixed: pol = 1; break;
      case NormalizationPolicy::Percentile: pol = 2; break;
    }
    const char* polNames[] = {"Auto", "Fixed", "Percentile"};
    if (ImGui::Combo("Policy", &pol, polNames, 3)) {
      ns.policy = pol == 0 ? NormalizationPolicy::Auto : (pol == 1 ? NormalizationPolicy::Fixed : NormalizationPolicy::Percentile);
    }
    if (ns.policy == NormalizationPolicy::Fixed) {
      ImGui::DragFloat("Min", &ns.minVal, 0.01f);
      ImGui::DragFloat("Max", &ns.maxVal, 0.01f);
      if (ns.maxVal <= ns.minVal) ns.maxVal = ns.minVal + 1.0f;
    } else if (ns.policy == NormalizationPolicy::Percentile) {
      ImGui::DragFloat("Low %", &ns.lowPct, 0.1f, 0.0f, 100.0f);
      ImGui::DragFloat("High %", &ns.highPct, 0.1f, 0.0f, 100.0f);
      if (ns.highPct < ns.lowPct) std::swap(ns.lowPct, ns.highPct);
    }
    if (ImGui::Button("Apply Normalization")) {
      enqueueCmd(std::make_shared<SetNormalization2D>(selName, ns));
      invalidated.store(true, std::memory_order_release);
    }
  }
#else
  (void)fbw; (void)fbh; (void)channels; (void)enqueueCmd; (void)invalidated; (void)zoomMin; (void)zoomMax;
#endif
}

} // namespace Ravl2::DebugDisplay::Ui
