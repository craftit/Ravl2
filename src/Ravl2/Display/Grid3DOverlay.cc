#include "Ravl2/Display/Grid3DOverlay.hh"

#if defined(RAVL2_WITH_IMGUI)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#pragma GCC diagnostic pop
#endif

#include <algorithm>

namespace Ravl2::DebugDisplay::Render3D
{

  // Map NDC [-1,+1] to screen pixels (origin top-left)
  static inline ImVec2 ndcToScreen(const Eigen::Vector3f &ndc, const Viewport3DNode::Rect &rect) noexcept
  {
    const float sx = (ndc.x() * 0.5f + 0.5f) * static_cast<float>(rect.w);
    const float sy = (1.0f - (ndc.y() * 0.5f + 0.5f)) * static_cast<float>(rect.h);
    return ImVec2(static_cast<float>(rect.x) + sx, static_cast<float>(rect.y) + sy);
  }

  static inline bool projectToNdc(const Eigen::Matrix4f &vp,
                                  const Eigen::Vector3f &p,
                                  Eigen::Vector3f &ndcOut) noexcept
  {
    Eigen::Vector4f hp(p.x(), p.y(), p.z(), 1.0f);
    Eigen::Vector4f clip = vp * hp;
    // Simple reject if behind camera or w ~ 0
    if(clip.w() <= 0.0001f) return false;
    const float invW = 1.0f / clip.w();
    ndcOut.x() = clip.x() * invW;
    ndcOut.y() = clip.y() * invW;
    ndcOut.z() = clip.z() * invW;
    // Optional: clip against ndc cube; we allow slight overflow and let ImGui clip
    return true;
  }

  void drawGridImGui(const OrbitCamera &cam,
                     const Viewport3DNode::Rect &rect,
#if defined(RAVL2_WITH_IMGUI)
                     ImDrawList *drawList,
#else
                     void * /*drawList*/,
#endif
                     float cellSize,
                     int halfCells) noexcept
  {
#if defined(RAVL2_WITH_IMGUI)
    if(!drawList || rect.w <= 1 || rect.h <= 1) return;

    const Eigen::Matrix4f view = cam.buildViewMatrix();
    const Eigen::Matrix4f proj = cam.buildProjMatrix();
    const Eigen::Matrix4f vp = proj * view;

    const float extent = static_cast<float>(halfCells) * cellSize;

// Suppress -Wold-style-cast for IM_COL32 macro in this small block
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    const ImU32 colMajor = IM_COL32(180, 180, 180, 128);
    const ImU32 colMinor = IM_COL32(120, 120, 120, 96);
    const ImU32 colAxesX = IM_COL32(220, 64, 64, 160);
    const ImU32 colAxesZ = IM_COL32(64, 128, 220, 160);
#pragma GCC diagnostic pop

    // Draw lines parallel to Z (varying X)
    for(int i = -halfCells; i <= halfCells; ++i) {
      const float x = static_cast<float>(i) * cellSize;
      Eigen::Vector3f p0(x, 0.0f, -extent);
      Eigen::Vector3f p1(x, 0.0f, +extent);
      Eigen::Vector3f n0, n1;
      if(projectToNdc(vp, p0, n0) && projectToNdc(vp, p1, n1)) {
        const ImVec2 s0 = ndcToScreen(n0, rect);
        const ImVec2 s1 = ndcToScreen(n1, rect);
        const bool axis = (i == 0);
        const bool major = (i % 10 == 0);
        const ImU32 col = axis ? colAxesX : (major ? colMajor : colMinor);
        drawList->AddLine(s0, s1, col, 1.0f);
      }
    }

    // Draw lines parallel to X (varying Z)
    for(int j = -halfCells; j <= halfCells; ++j) {
      const float z = static_cast<float>(j) * cellSize;
      Eigen::Vector3f p0(-extent, 0.0f, z);
      Eigen::Vector3f p1(+extent, 0.0f, z);
      Eigen::Vector3f n0, n1;
      if(projectToNdc(vp, p0, n0) && projectToNdc(vp, p1, n1)) {
        const ImVec2 s0 = ndcToScreen(n0, rect);
        const ImVec2 s1 = ndcToScreen(n1, rect);
        const bool axis = (j == 0);
        const bool major = (j % 10 == 0);
        const ImU32 col = axis ? colAxesZ : (major ? colMajor : colMinor);
        drawList->AddLine(s0, s1, col, 1.0f);
      }
    }
#endif
  }

}// namespace Ravl2::DebugDisplay::Render3D
