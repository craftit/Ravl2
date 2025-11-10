#include "Ravl2/Display/InputController2D.hh"

#include <algorithm>
#include <cmath>

namespace Ravl2::DebugDisplay {

void InputController2D::onMouseButtonDown(int x, int y,
                                          const std::unordered_map<std::string, SDL_FRect>& lastRects,
                                          const std::unordered_map<std::string, SDL_FRect>& contentRects) noexcept
{
  mDragging = false;
  mActiveChannel.clear();
  mLastX = x; mLastY = y;
  const float fx = static_cast<float>(x);
  const float fy = static_cast<float>(y);
  // Only start drag if inside both the image rect and the window content rect
  for (const auto &kv : lastRects) {
    const auto &img = kv.second;
    auto itC = contentRects.find(kv.first);
    if (itC == contentRects.end()) continue;
    const auto &c = itC->second;
    const bool inImg = (fx >= img.x && fx < img.x + img.w && fy >= img.y && fy < img.y + img.h);
    const bool inContent = (fx >= c.x && fx < c.x + c.w && fy >= c.y && fy < c.y + c.h);
    if (inImg && inContent) {
      mActiveChannel = kv.first;
      mDragging = true;
      break;
    }
  }
}

void InputController2D::onMouseButtonDown(int x, int y,
                                          const std::string& channelName,
                                          const std::unordered_map<std::string, SDL_FRect>& lastRects,
                                          const std::unordered_map<std::string, SDL_FRect>& contentRects) noexcept
{
  mDragging = false;
  mActiveChannel.clear();
  mLastX = x; mLastY = y;
  const float fx = static_cast<float>(x);
  const float fy = static_cast<float>(y);
  auto itImg = lastRects.find(channelName);
  auto itCR  = contentRects.find(channelName);
  if (itImg == lastRects.end() || itCR == contentRects.end()) return;
  const auto &img = itImg->second;
  const auto &c   = itCR->second;
  const bool inImg = (fx >= img.x && fx < img.x + img.w && fy >= img.y && fy < img.y + img.h);
  const bool inContent = (fx >= c.x && fx < c.x + c.w && fy >= c.y && fy < c.y + c.h);
  if (inImg && inContent) {
    mActiveChannel = channelName;
    mDragging = true;
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
                                     const std::unordered_map<std::string, SDL_FRect>& contentRects,
                                     ChannelRegistry& channels) noexcept
{
  // Determine channel under cursor and fetch its rects
  std::string under;
  SDL_FRect rect{};
  SDL_FRect cRect{};
  const float fx = static_cast<float>(mouseX);
  const float fy = static_cast<float>(mouseY);
  for (const auto &kv : lastRects) {
    const auto &r = kv.second;
    auto itC = contentRects.find(kv.first);
    if (itC == contentRects.end()) continue;
    const auto &cr = itC->second;
    const bool inImg = (fx >= r.x && fx < r.x + r.w && fy >= r.y && fy < r.y + r.h);
    const bool inContent = (fx >= cr.x && fx < cr.x + cr.w && fy >= cr.y && fy < cr.y + cr.h);
    if (inImg && inContent) { under = kv.first; rect = r; cRect = cr; break; }
  }
  if (under.empty()) return;

  auto &ch = channels.getOrCreateChannel(under);
  auto &view = ch.view2D;
  float &sx = view.scaleVector()[0];
  float &sy = view.scaleVector()[1];
  float &tx = view.translation()[0];
  float &ty = view.translation()[1];

  // Use continuous zoom factor; support multiple wheel ticks per event
  constexpr float kFactor = 1.1f;
  int delta = wheelY;
  if (delta == 0) return;
  // Clamp excessive deltas to avoid huge jumps from high-resolution wheels
  delta = std::clamp(delta, -8, 8);
  const float factor = std::pow(kFactor, static_cast<float>(delta));

  const float newSx = std::clamp(sx * factor, mZoomMin, mZoomMax);
  const float newSy = std::clamp(sy * factor, mZoomMin, mZoomMax);

  // Anchor around mouse using content-origin-based mapping: screen = contentMin + tx + ix*s
  constexpr float kEps = 1e-6f;
  const float curSx = (std::abs(sx) < kEps) ? (sx >= 0 ? kEps : -kEps) : sx;
  const float curSy = (std::abs(sy) < kEps) ? (sy >= 0 ? kEps : -kEps) : sy;
  const float ix = (fx - (cRect.x + tx)) / curSx;
  const float iy = (fy - (cRect.y + ty)) / curSy;

  // Keep the point under cursor fixed
  tx = fx - (cRect.x + ix * newSx);
  ty = fy - (cRect.y + iy * newSy);
  sx = newSx; sy = newSy;

  invalidate();
}

void InputController2D::onMouseWheel(int wheelY, int mouseX, int mouseY,
                                     const std::string& channelName,
                                     const std::unordered_map<std::string, SDL_FRect>& lastRects,
                                     const std::unordered_map<std::string, SDL_FRect>& contentRects,
                                     ChannelRegistry& channels) noexcept
{
  if (channelName.empty()) return;
  auto itImg = lastRects.find(channelName);
  auto itCR  = contentRects.find(channelName);
  if (itImg == lastRects.end() || itCR == contentRects.end()) return;
  const float fx = static_cast<float>(mouseX);
  const float fy = static_cast<float>(mouseY);
  const auto &r = itImg->second;
  const auto &cr = itCR->second;
  const bool inImg = (fx >= r.x && fx < r.x + r.w && fy >= r.y && fy < r.y + r.h);
  const bool inContent = (fx >= cr.x && fx < cr.x + cr.w && fy >= cr.y && fy < cr.y + cr.h);
  if (!(inImg && inContent)) return;

  auto &ch = channels.getOrCreateChannel(channelName);
  auto &view = ch.view2D;
  float &sx = view.scaleVector()[0];
  float &sy = view.scaleVector()[1];
  float &tx = view.translation()[0];
  float &ty = view.translation()[1];

  constexpr float kFactor = 1.1f;
  int delta = wheelY;
  if (delta == 0) return;
  delta = std::clamp(delta, -8, 8);
  const float factor = std::pow(kFactor, static_cast<float>(delta));

  const float newSx = std::clamp(sx * factor, mZoomMin, mZoomMax);
  const float newSy = std::clamp(sy * factor, mZoomMin, mZoomMax);

  constexpr float kEps = 1e-6f;
  const float curSx = (std::abs(sx) < kEps) ? (sx >= 0 ? kEps : -kEps) : sx;
  const float curSy = (std::abs(sy) < kEps) ? (sy >= 0 ? kEps : -kEps) : sy;
  const float ix = (fx - (cr.x + tx)) / curSx;
  const float iy = (fy - (cr.y + ty)) / curSy;

  tx = fx - (cr.x + ix * newSx);
  ty = fy - (cr.y + iy * newSy);
  sx = newSx; sy = newSy;

  invalidate();
}

} // namespace Ravl2::DebugDisplay
