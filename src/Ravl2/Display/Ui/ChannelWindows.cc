#include "Ravl2/Display/Ui/ChannelWindows.hh"
#include "Ravl2/Display/DebugDisplay.hh"

#if defined(RAVL2_WITH_BGFX)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#include "Ravl2/Display/ThirdParty/bgfx_imgui/ImGUI/imgui.hh"
#pragma GCC diagnostic pop
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <implot.h>
#pragma GCC diagnostic pop

#include <SDL2/SDL.h>

#include "Ravl2/Display/ISceneNode.hh"
#include "Ravl2/Display/Image2DNode.hh"
#include "Ravl2/Display/Image2DNodeBase.hh"
#include "Ravl2/Display/CompositeNode.hh"
#include "Ravl2/Display/Viewport3DNode.hh"
#include <algorithm>
#include <cmath>
#include <limits>
#include "Ravl2/Types.hh"
#include "Ravl2/Display/Grid3DOverlay.hh"

namespace Ravl2::DebugDisplay::Ui::ChannelWindows
{

  void build(uint16_t fbw, uint16_t fbh,
             ChannelRegistry &channels,
             std::unordered_map<std::string, SDL_FRect> &lastRects,
             std::unordered_map<std::string, SDL_FPoint> &imageOrigins,
             std::unordered_map<std::string, SDL_FRect> &contentRects,
             std::atomic_bool &invalidated,
             std::string &hoveredChannelOut,
             std::string &hoveredImageChannelOut)
  {
    (void)fbw;
    (void)fbh;
    lastRects.clear();
    imageOrigins.clear();
    contentRects.clear();
    hoveredChannelOut.clear();
    hoveredImageChannelOut.clear();
    // In headless mode we don't render any UI. Returning early keeps
    // bookkeeping maps cleared so tests can run without a window.
    if(DebugDisplay::isHeadless()) {
      return;
    }
    RenderContext rc {};
    rc.framebufferWidth = fbw;
    rc.framebufferHeight = fbh;
    channels.forEachChannel([&](ChannelState &ch) {
      // Disable scrollbars to avoid interaction with image pan/zoom.
      ImGuiWindowFlags wflags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
      if(!ImGui::Begin(ch.name.c_str(), nullptr, wflags)) {
        ImGui::End();
        return;
      }

      // Compute content rect in screen space for input gating
      const ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
      const ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
      const ImVec2 winPos = ImGui::GetWindowPos();
      const ImVec2 contentMinScreen = ImVec2(winPos.x + contentMin.x, winPos.y + contentMin.y);
      const ImVec2 contentMaxScreen = ImVec2(winPos.x + contentMax.x, winPos.y + contentMax.y);
      // Use the content min as the image origin so translation is relative to the content area
      SDL_FPoint origin {contentMinScreen.x, contentMinScreen.y};
      SDL_FRect cRect {contentMinScreen.x, contentMinScreen.y,
                       contentMaxScreen.x - contentMinScreen.x,
                       contentMaxScreen.y - contentMinScreen.y};
      imageOrigins[ch.name] = origin;
      contentRects[ch.name] = cRect;

      // Optional developer-only view mode selector (hidden by default)
      if(ch.flags.showViewToggle) {
        int viewModeInt = (ch.viewMode == ViewMode::View2D) ? 0 : 1;
        ImGui::Separator();
        ImGui::TextUnformatted("View:");
        ImGui::SameLine();
        bool sel2d = ImGui::RadioButton("2D", viewModeInt == 0);
        ImGui::SameLine();
        bool sel3d = ImGui::RadioButton("3D", viewModeInt == 1);
        if(sel2d) {
          ch.viewMode = ViewMode::View2D;
        }
        if(sel3d) {
          ch.viewMode = ViewMode::View3D;
        }
      }

      const bool enable2D = (ch.viewMode == ViewMode::View2D);
      const bool enable3D = (ch.viewMode == ViewMode::View3D);

      // Auto-focus channel window after receiving first 3D payload
      if(enable3D && ch.flags.wantsFocus3D) {
        ImGui::SetWindowFocus();
        ch.flags.wantsFocus3D = false;
      }

      // If this window (and its children) are hovered, record it as the top-most hovered channel.
      if(ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
        hoveredChannelOut = ch.name;
      }

      // --- Phase 7: Plot rendering ---
      // If this channel has plot data, render it in the channel window
      // Check if channel has ONLY plot data (no 2D/3D content)
      const bool hasOnlyPlotData = (ch.plotState.has_value() && !ch.plotState->series.empty() && !ch.sceneContent);

      if(hasOnlyPlotData) {
        PlotState &plotState = ch.plotState.value();

        // Toolbar for plot controls
        if(ImGui::Button("Fit")) {
          ImPlot::SetNextAxesToFit();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Follow", &plotState.followMode);
        ImGui::SameLine();
        ImGui::Text("Zoom: Mouse wheel | Pan: Right-click drag");

        // Use full available content area for the plot
        // Note: Use unique ID per channel to maintain separate zoom/pan state
        std::string plotId = "##plot_" + ch.name;
        if(ImPlot::BeginPlot(plotId.c_str(), ImVec2(-1, -1))) {
          ImPlot::SetupAxes(plotState.xAxisLabel.c_str(), plotState.yAxisLabel.c_str());

          // If follow mode is enabled, compute the latest x range and set axis limits
          if(plotState.followMode) {
            // Find the maximum x value across all series
            double maxX = -std::numeric_limits<double>::infinity();
            for(const auto &[name, series] : plotState.series) {
              if(!series.x.empty()) {
                double seriesMaxX = static_cast<double>(series.x.back());
                if(seriesMaxX > maxX) {
                  maxX = seriesMaxX;
                }
              }
            }

            if(std::isfinite(maxX)) {
              // Show a fixed window width of the most recent data
              // Adjust this window size as needed (e.g., last 100 x-units)
              double windowWidth = 100.0;
              double minX = maxX - windowWidth;
              ImPlot::SetupAxisLimits(ImAxis_X1, minX, maxX, ImGuiCond_Always);
            }
          }

          for(const auto &[name, series] : plotState.series) {
            if(series.x.empty() || series.y.empty()) continue;

            const char *label = series.label.empty() ? name.c_str() : series.label.c_str();
            int size = std::min(static_cast<int>(series.x.size()), static_cast<int>(series.y.size()));
            ImPlot::PlotLine(label, series.x.data(), series.y.data(), size);
          }

          ImPlot::EndPlot();
        }
      } else if(enable2D) {
        // Small toolbar: Reset and Fit using the window's content region
        if(ImGui::Button("Reset")) {
          ch.view2D = ScaleTranslate<float, 2>::identity();
          invalidated.store(true, std::memory_order_release);
        }
        ImGui::SameLine();
        bool doFit = ImGui::Button("Fit");

        if(ch.sceneContent) {
          // Prepare the scene content (could be Image2DNode or CompositeNode)
          ch.sceneContent->prepare(rc);

          // Try to find an Image2DNodeBase to determine dimensions for fit/pan/zoom
          Image2DNodeBase *imageNode = nullptr;
          if(auto *img = dynamic_cast<Image2DNodeBase *>(ch.sceneContent.get())) {
            imageNode = img;
          } else if(auto *composite = dynamic_cast<CompositeNode *>(ch.sceneContent.get())) {
            // If it's a CompositeNode, the base image is typically the first child
            if(composite->childCount() > 0) {
              imageNode = dynamic_cast<Image2DNodeBase *>(composite->children()[0].get());
            }
          }

#if defined(RAVL2_WITH_BGFX)
          if(imageNode && imageNode->width > 0 && imageNode->height > 0 && imageNode->textureHandleIdx != UINT16_MAX) {
            if(doFit) {
              // Compute fit using the full content region size, not the remaining avail after the toolbar
              const float contentW = cRect.w;
              const float contentH = cRect.h;
              if(contentW > 1.0f && contentH > 1.0f) {
                const float imgWf = static_cast<float>(imageNode->width);
                const float imgHf = static_cast<float>(imageNode->height);
                const float sFit = std::max(0.0001f, std::min(contentW / imgWf, contentH / imgHf));
                auto v = ch.view2D.scaleVector();
                v[0] = sFit;
                v[1] = sFit;
                ch.view2D.scale(v);
                auto tr = ch.view2D.translation();
                tr[0] = (contentW - imgWf * sFit) * 0.5f;
                tr[1] = (contentH - imgHf * sFit) * 0.5f;
                ch.view2D.translate(tr);
                invalidated.store(true, std::memory_order_release);
              }
            }
            bgfx::TextureHandle thdl {imageNode->textureHandleIdx};
            const float sx = ch.view2D.scaleVector()[0];
            const float sy = ch.view2D.scaleVector()[1];
            const float tx = ch.view2D.translation()[0];
            const float ty = ch.view2D.translation()[1];
            // Compute position in screen space anchored at the content origin
            ImVec2 pos = ImVec2(origin.x + tx, origin.y + ty);
            ImVec2 size = ImVec2(static_cast<float>(imageNode->width) * sx, static_cast<float>(imageNode->height) * sy);
            // Set cursor to the computed position and draw
            ImGui::SetCursorScreenPos(pos);
            ImGui::Image(thdl, size);
            // Record the full image rect (screen space)
            SDL_FRect imgRect {pos.x, pos.y, size.x, size.y};
            lastRects[ch.name] = imgRect;
            // If the drawn image item is hovered, remember this channel as the top-most hovered image
            if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
              hoveredImageChannelOut = ch.name;

              // Display pixel information tooltip
              // Note: We do this inline instead of using PixelInspector2D to avoid deadlock
              // (we're already inside forEachChannel which holds the registry lock)
              if(ch.sceneContent && ch.sceneContent->supportsPixelQuery()) {
                ImGuiIO &io = ImGui::GetIO();
                const float fx = io.MousePos.x;
                const float fy = io.MousePos.y;

                // Map mouse to image pixel using image origin + view2D transform (reuse sx,sy,tx,ty from above)
                const float denomX = (sx != 0.0f) ? sx : 1.0f;
                const float denomY = (sy != 0.0f) ? sy : 1.0f;
                const int ix = static_cast<int>((fx - (origin.x + tx)) / denomX);
                const int iy = static_cast<int>((fy - (origin.y + ty)) / denomY);

                // Query pixel information
                PixelQueryResult queryResult = ch.sceneContent->queryPixelInfo(ix, iy);
                if(queryResult.valid) {
                  ImGui::BeginTooltip();
                  ImGui::Text("%s", queryResult.coordinateText.c_str());
                  ImGui::Text("%s", queryResult.valueText.c_str());
                  if(queryResult.extraInfo) {
                    ImGui::Separator();
                    ImGui::Text("%s", queryResult.extraInfo->c_str());
                  }
                  ImGui::EndTooltip();
                }
              }
            }
          }

          // Populate RenderContext with overlay rendering information
          rc.imguiDrawList = ImGui::GetWindowDrawList();
          rc.origin = origin;
          rc.view2D = ch.view2D;

          // Render the scene content (includes overlays if using CompositeNode)
          ch.sceneContent->render(rc);
#endif
        }
      }

      // --- Phase 6a: 3D Viewport scaffolding with OrbitCamera input mapping ---
      if(enable3D) {
        if(!ch.sceneContent) {
          ch.sceneContent = std::make_unique<Viewport3DNode>();
        }
        auto *vp3d = dynamic_cast<Viewport3DNode *>(ch.sceneContent.get());
        if(!vp3d) {
          // Scene content exists but isn't a Viewport3DNode, replace it
          ch.sceneContent = std::make_unique<Viewport3DNode>();
          vp3d = static_cast<Viewport3DNode *>(ch.sceneContent.get());
        }

        // Full-window 3D viewport child (fills content region)
        ImGui::BeginChild("##3d_view", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Compute the child rect in screen space
        const ImVec2 childPos = ImGui::GetWindowPos();
        const ImVec2 childSize = ImGui::GetWindowSize();
        vp3d->setViewportRect(static_cast<int>(childPos.x), static_cast<int>(childPos.y),
                              static_cast<int>(childSize.x), static_cast<int>(childSize.y));

        // Small toolbar row inside the 3D view (top-left)
        ImGui::SetCursorScreenPos(ImVec2(childPos.x + 8.0f, childPos.y + 8.0f));
        if(ImGui::Button("Reset")) {
          vp3d->camera.reset();
          invalidated.store(true, std::memory_order_release);
        }
        ImGui::SameLine();
        bool doFit3D = ImGui::Button("Fit");
        (void)doFit3D;// TODO in 6e

        // Input mapping: no Alt required inside the 3D child
        ImGuiIO &io = ImGui::GetIO();
        const bool hovered3D = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_NoPopupHierarchy);
        if(hovered3D) {
          bool changed = false;
          // Orbit: LMB drag
          if(io.MouseDown[0]) {
            const float sensitivity = 0.005f;// radians per pixel
            float dyaw = -io.MouseDelta.x * sensitivity;
            float dpitch = -io.MouseDelta.y * sensitivity;
            vp3d->camera.orbit(dyaw, dpitch);
            changed = true;
          }
          // Pan: MMB drag
          if(io.MouseDown[2]) {
            const float panScale = 0.002f * vp3d->camera.distance;
            float dx = -io.MouseDelta.x * panScale;
            float dy = io.MouseDelta.y * panScale;
            vp3d->camera.pan(dx, dy);
            changed = true;
          }
          // Dolly: mouse wheel
          if(io.MouseWheel != 0.0f) {
            const float dollyScale = 0.1f;
            vp3d->camera.dolly(-io.MouseWheel * dollyScale);
            changed = true;
          }
          if(changed) {
            invalidated.store(true, std::memory_order_release);
          }
        }

        // Draw 3D grid overlay using CPU projection (always visible)
        {
          ImDrawList *dl = ImGui::GetWindowDrawList();
          if(dl) {
            Render3D::drawGridImGui(vp3d->camera, vp3d->rect, dl, vp3d->gridCellSize, vp3d->gridHalfCells);
          }
        }

        // Minimal HUD to visualize current camera values (temporary)
        ImGui::SetCursorScreenPos(ImVec2(childPos.x + 8.0f, childPos.y + 36.0f));
        ImGui::Text("Cam: yaw=%.2f pitch=%.2f dist=%.2f fovY=%.1fdeg aspect=%.2f",
                    static_cast<double>(vp3d->camera.yaw), static_cast<double>(vp3d->camera.pitch),
                    static_cast<double>(vp3d->camera.distance), static_cast<double>(vp3d->camera.fovY * 180.0f / 3.14159265f), static_cast<double>(vp3d->camera.aspect));

        ImGui::EndChild();
      }

      ImGui::End();
    });
  }

}// namespace Ravl2::DebugDisplay::Ui::ChannelWindows
