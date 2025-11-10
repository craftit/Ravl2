#include "Ravl2/Display/InputController2D.hh"

#include <algorithm>

namespace Ravl2::DebugDisplay {

void InputController2D::onMouseButtonDown(int x, int y,
                                          const std::unordered_map<std::string, SDL_FRect>& lastRects) noexcept
{
  mDragging = true;
  mLastX = x; mLastY = y;
  mActiveChannel.clear();
  const float fx = static_cast<float>(x);
  const float fy = static_cast<float>(y);
  for (const auto &kv : lastRects) {
    const auto &r = kv.second;
    if (fx >= r.x && fx < r.x + r.w && fy >= r.y && fy < r.y + r.h) {
      mActiveChannel = kv.first;
      break;
    }
  }
}

void InputController2D::onMouseButtonUp(uint8_t sdlButton) noexcept
{
  if (sdlButton == SDL_BUTTON_LEFT) {
    mDragging = false;
  }
}

void InputController2D::onMouseMotion(int x, int y,
                                      ChannelRegistry& channels) noexcept
{
  if (!mDragging || mActiveChannel.empty()) return;
  const int dx = x - mLastX;
  const int dy = y - mLastY;
  mLastX = x; mLastY = y;

  auto &ch = channels.getOrCreateChannel(mActiveChannel);
  auto &t = ch.view2D.translation();
  t[0] += static_cast<float>(dx);
  t[1] += static_cast<float>(dy);
  invalidate();
}

void InputController2D::onMouseWheel(int wheelY, int mouseX, int mouseY,
                                     const std::unordered_map<std::string, SDL_FRect>& lastRects,
                                     ChannelRegistry& channels) noexcept
{
  // Determine channel under cursor
  std::string under;
  const float fx = static_cast<float>(mouseX);
  const float fy = static_cast<float>(mouseY);
  for (const auto &kv : lastRects) {
    const auto &r = kv.second;
    if (fx >= r.x && fx < r.x + r.w && fy >= r.y && fy < r.y + r.h) { under = kv.first; break; }
  }
  if (under.empty()) return;

  auto &ch = channels.getOrCreateChannel(under);
  auto &view = ch.view2D;
  float &sx = view.scaleVector()[0];
  float &sy = view.scaleVector()[1];
  float &tx = view.translation()[0];
  float &ty = view.translation()[1];

  constexpr float kFactor = 1.1f;
  const float factor = (wheelY > 0) ? kFactor : 1.0f / kFactor;

  const float newSx = std::clamp(sx * factor, mZoomMin, mZoomMax);
  const float newSy = std::clamp(sy * factor, mZoomMin, mZoomMax);

  // Compute image coordinates under cursor with old transform
  const float ix = (static_cast<float>(mouseX) - tx) / (sx != 0.0f ? sx : 1.0f);
  const float iy = (static_cast<float>(mouseY) - ty) / (sy != 0.0f ? sy : 1.0f);

  // Update translation so the point under cursor remains fixed
  tx = static_cast<float>(mouseX) - ix * newSx;
  ty = static_cast<float>(mouseY) - iy * newSy;
  sx = newSx; sy = newSy;

  invalidate();
}

} // namespace Ravl2::DebugDisplay
