#include "Ravl2/Display/Ui/ChannelWindows.hh"

#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#include "Ravl2/Display/bgfx_imgui/ImGUI/imgui.hh"
#pragma GCC diagnostic pop
#endif

#include <SDL2/SDL.h>

#include "Ravl2/Display/Image2DNode.hh"
#include <algorithm>

namespace Ravl2::DebugDisplay::Ui::ChannelWindows {

void build(uint16_t fbw, uint16_t fbh,
           ChannelRegistry& channels,
           std::unordered_map<std::string, SDL_FRect>& lastRects,
           std::atomic_bool& invalidated)
{
  (void)fbw; (void)fbh;
  lastRects.clear();
  RenderContext rc{}; rc.framebufferWidth = fbw; rc.framebufferHeight = fbh;
  channels.forEachChannel([&](ChannelState &ch){
#if defined(RAVL2_WITH_IMGUI)
    if (!ImGui::Begin(ch.name.c_str())) { ImGui::End(); return; }

    // Small toolbar: Reset and Fit using the window's content region
    if (ImGui::Button("Reset")) {
      ch.view2D = ScaleTranslate<float,2>::identity();
      invalidated.store(true, std::memory_order_release);
    }
    ImGui::SameLine();
    bool doFit = ImGui::Button("Fit");

    if (ch.baseImage2D) {
      auto *node = static_cast<Image2DNode*>(ch.baseImage2D.get());
      node->prepare(rc);
  #if defined(RAVL2_WITH_BGFX)
      if (node->width > 0 && node->height > 0 && node->textureHandleIdx != UINT16_MAX) {
        if (doFit) {
          // Compute fit using the current content region (remaining space)
          ImVec2 avail = ImGui::GetContentRegionAvail();
          if (avail.x > 1.0f && avail.y > 1.0f) {
            const float imgWf = static_cast<float>(node->width);
            const float imgHf = static_cast<float>(node->height);
            const float sFit = std::max(0.0001f, std::min(avail.x / imgWf, avail.y / imgHf));
            auto v = ch.view2D.scaleVector(); v[0] = sFit; v[1] = sFit; ch.view2D.scale(v);
            auto tr = ch.view2D.translation();
            tr[0] = (avail.x - imgWf * sFit) * 0.5f;
            tr[1] = (avail.y - imgHf * sFit) * 0.5f;
            ch.view2D.translate(tr);
            invalidated.store(true, std::memory_order_release);
          }
        }
        bgfx::TextureHandle thdl{node->textureHandleIdx};
        const float sx = ch.view2D.scaleVector()[0];
        const float sy = ch.view2D.scaleVector()[1];
        const float tx = ch.view2D.translation()[0];
        const float ty = ch.view2D.translation()[1];
        // Compute position in screen space (relative to content region)
        ImVec2 winPos = ImGui::GetCursorScreenPos();
        ImVec2 pos = ImVec2(winPos.x + tx, winPos.y + ty);
        ImVec2 size = ImVec2(static_cast<float>(node->width) * sx, static_cast<float>(node->height) * sy);
        // Set cursor and draw
        ImGui::SetCursorScreenPos(pos);
        ImGui::Image(thdl, size);
        // Update hit-test rect in screen space
        SDL_FRect r{ pos.x, pos.y, size.x, size.y };
        lastRects[ch.name] = r;
      }
  #endif
    }
    ImGui::End();
#else
    (void)ch; // UI disabled
#endif
  });
}

} // namespace Ravl2::DebugDisplay::Ui::ChannelWindows
