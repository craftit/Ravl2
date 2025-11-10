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
#include "Ravl2/Display/Viewport3DNode.hh"
#include "Ravl2/Display/Overlays/OverlayRenderer2D.hh"
#include <algorithm>
#include <cmath>
#include "Ravl2/Types.hh"
#include "Ravl2/Display/3D/Grid3DOverlay.hh"

namespace Ravl2::DebugDisplay::Ui::ChannelWindows {

void build(uint16_t fbw, uint16_t fbh,
           ChannelRegistry& channels,
           std::unordered_map<std::string, SDL_FRect>& lastRects,
           std::unordered_map<std::string, SDL_FPoint>& imageOrigins,
           std::unordered_map<std::string, SDL_FRect>& contentRects,
           std::atomic_bool& invalidated,
           std::string& hoveredChannelOut,
           std::string& hoveredImageChannelOut)
{
  (void)fbw; (void)fbh;
  lastRects.clear();
  imageOrigins.clear();
  contentRects.clear();
  hoveredChannelOut.clear();
  hoveredImageChannelOut.clear();
  RenderContext rc{}; rc.framebufferWidth = fbw; rc.framebufferHeight = fbh;
  channels.forEachChannel([&](ChannelState &ch){
#if defined(RAVL2_WITH_IMGUI)
    // Disable scrollbars to avoid interaction with image pan/zoom.
    ImGuiWindowFlags wflags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (!ImGui::Begin(ch.name.c_str(), nullptr, wflags)) { ImGui::End(); return; }

    // Compute content rect in screen space for input gating
    const ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    const ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    const ImVec2 winPos = ImGui::GetWindowPos();
    const ImVec2 contentMinScreen = ImVec2(winPos.x + contentMin.x, winPos.y + contentMin.y);
    const ImVec2 contentMaxScreen = ImVec2(winPos.x + contentMax.x, winPos.y + contentMax.y);
    // Use the content min as the image origin so translation is relative to the content area
    SDL_FPoint origin{ contentMinScreen.x, contentMinScreen.y };
    SDL_FRect cRect{ contentMinScreen.x, contentMinScreen.y,
                     contentMaxScreen.x - contentMinScreen.x,
                     contentMaxScreen.y - contentMinScreen.y };
    imageOrigins[ch.name] = origin;
    contentRects[ch.name] = cRect;

    // View mode selector: 2D or 3D exclusively per channel
    int viewModeInt = (ch.viewMode == ViewMode::View2D) ? 0 : 1;
    ImGui::Separator();
    ImGui::TextUnformatted("View:");
    ImGui::SameLine();
    bool sel2d = ImGui::RadioButton("2D", viewModeInt == 0);
    ImGui::SameLine();
    bool sel3d = ImGui::RadioButton("3D", viewModeInt == 1);
    if (sel2d) { ch.viewMode = ViewMode::View2D; }
    if (sel3d) { ch.viewMode = ViewMode::View3D; }

    const bool enable2D = (ch.viewMode == ViewMode::View2D);
    const bool enable3D = (ch.viewMode == ViewMode::View3D);

    // If this window (and its children) are hovered, record it as the top-most hovered channel.
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
      hoveredChannelOut = ch.name;
    }

    if (enable2D) {
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
          // Compute fit using the full content region size, not the remaining avail after the toolbar
          const float contentW = cRect.w;
          const float contentH = cRect.h;
          if (contentW > 1.0f && contentH > 1.0f) {
            const float imgWf = static_cast<float>(node->width);
            const float imgHf = static_cast<float>(node->height);
            const float sFit = std::max(0.0001f, std::min(contentW / imgWf, contentH / imgHf));
            auto v = ch.view2D.scaleVector(); v[0] = sFit; v[1] = sFit; ch.view2D.scale(v);
            auto tr = ch.view2D.translation();
            tr[0] = (contentW - imgWf * sFit) * 0.5f;
            tr[1] = (contentH - imgHf * sFit) * 0.5f;
            ch.view2D.translate(tr);
            invalidated.store(true, std::memory_order_release);
          }
        }
        bgfx::TextureHandle thdl{node->textureHandleIdx};
        const float sx = ch.view2D.scaleVector()[0];
        const float sy = ch.view2D.scaleVector()[1];
        const float tx = ch.view2D.translation()[0];
        const float ty = ch.view2D.translation()[1];
        // Compute position in screen space anchored at the content origin
        ImVec2 pos = ImVec2(origin.x + tx, origin.y + ty);
        ImVec2 size = ImVec2(static_cast<float>(node->width) * sx, static_cast<float>(node->height) * sy);
        // Set cursor to the computed position and draw
        ImGui::SetCursorScreenPos(pos);
        ImGui::Image(thdl, size);
        // Record the full image rect (screen space)
        SDL_FRect imgRect{ pos.x, pos.y, size.x, size.y };
        lastRects[ch.name] = imgRect;
        // If the drawn image item is hovered, remember this channel as the top-most hovered image
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
          hoveredImageChannelOut = ch.name;
        }

        // Render any registered overlays using ImGui draw list
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (drawList) {
          for (const auto& ov : ch.overlays) {
            if (ov) {
              ov->render(drawList, origin, ch.view2D, node->width, node->height);
            }
          }
        }
      }
  #endif
    }

    }

    // --- Phase 6a: 3D Viewport scaffolding with OrbitCamera input mapping ---
    if (enable3D) {
    if (!ch.viewport3D) {
      ch.viewport3D = std::make_unique<Viewport3DNode>();
    }
    auto *vp3d = static_cast<Viewport3DNode*>(ch.viewport3D.get());

  #if defined(RAVL2_WITH_IMGUI)
    if (ImGui::CollapsingHeader("3D View (Experimental)", ImGuiTreeNodeFlags_DefaultOpen)) {
      // Reserve a child region for the 3D viewport
      const float desiredHeight = std::max(120.0f, ImGui::GetTextLineHeightWithSpacing() * 12.0f);
      ImGui::BeginChild("##3d_view_child", ImVec2(0, desiredHeight), true, ImGuiWindowFlags_NoScrollbar);

      // Compute the child rect in screen space
      const ImVec2 childPos = ImGui::GetWindowPos();
      const ImVec2 childSize = ImGui::GetWindowSize();
      vp3d->setViewportRect(static_cast<int>(childPos.x), static_cast<int>(childPos.y),
                            static_cast<int>(childSize.x), static_cast<int>(childSize.y));

      // Toolbar for 3D: Reset and Fit (Fit is stubbed for now)
      if (ImGui::Button("Reset 3D")) {
        vp3d->camera.reset();
        invalidated.store(true, std::memory_order_release);
      }
      ImGui::SameLine();
      bool doFit3D = ImGui::Button("Fit 3D"); (void)doFit3D; // TODO in 6e

      // Input mapping: Alt modifies camera controls inside the child region
      ImGuiIO &io = ImGui::GetIO();
      const bool hovered3D = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
      if (hovered3D && io.KeyAlt) {
        bool changed = false;
        // Orbit: Alt + LMB drag → adjust yaw/pitch
        if (io.MouseDown[0]) {
          const float sensitivity = 0.005f; // radians per pixel
          float dyaw = -io.MouseDelta.x * sensitivity;
          float dpitch = -io.MouseDelta.y * sensitivity;
          vp3d->camera.orbit(dyaw, dpitch);
          changed = true;
        }
        // Pan: Alt + MMB drag
        if (io.MouseDown[2]) {
          const float panScale = 0.002f * vp3d->camera.distance;
          float dx = -io.MouseDelta.x * panScale;
          float dy = io.MouseDelta.y * panScale;
          vp3d->camera.pan(dx, dy);
          changed = true;
        }
        // Dolly: Alt + wheel (or just wheel?) — accept wheel while Alt is down
        if (io.MouseWheel != 0.0f) {
          const float dollyScale = 0.1f;
          vp3d->camera.dolly(-io.MouseWheel * dollyScale);
          changed = true;
        }
        if (changed) {
          invalidated.store(true, std::memory_order_release);
        }
      }

      // Draw 3D grid overlay using CPU projection (always visible)
      {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
          Render3D::drawGridImGui(vp3d->camera, vp3d->rect, dl, vp3d->gridCellSize, vp3d->gridHalfCells);
        }
      }

      // Minimal HUD to visualize current camera values (temporary)
      ImGui::Separator();
      ImGui::Text("Cam: yaw=%.2f pitch=%.2f dist=%.2f fovY=%.1fdeg aspect=%.2f", static_cast<double>(vp3d->camera.yaw), static_cast<double>(vp3d->camera.pitch),
                  static_cast<double>(vp3d->camera.distance), static_cast<double>(vp3d->camera.fovY * 180.0f / 3.14159265f), static_cast<double>(vp3d->camera.aspect));

      ImGui::EndChild();
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
