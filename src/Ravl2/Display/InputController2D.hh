#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <unordered_map>

#include <SDL2/SDL.h>

#include "Ravl2/Display/Channel.hh"

namespace Ravl2::DebugDisplay {

//! Simple per-session input controller for 2D pan/zoom.
//! Owns drag state and active channel selection; stateless channel data lives in ChannelRegistry.
class InputController2D {
public:
  InputController2D(std::atomic_bool& invalidated,
                    float zoomMin,
                    float zoomMax) noexcept
    : mInvalidated(invalidated), mZoomMin(zoomMin), mZoomMax(zoomMax) {}

  void setZoomLimits(float minVal, float maxVal) noexcept { mZoomMin = minVal; mZoomMax = maxVal; }

  void onMouseButtonDown(int x, int y,
                         const std::unordered_map<std::string, SDL_FRect>& lastRects,
                         const std::unordered_map<std::string, SDL_FRect>& contentRects) noexcept;

  void onMouseButtonUp(uint8_t sdlButton) noexcept;

  void onMouseMotion(int x, int y,
                     ChannelRegistry& channels) noexcept;

  void onMouseWheel(int wheelY, int mouseX, int mouseY,
                    const std::unordered_map<std::string, SDL_FRect>& lastRects,
                    const std::unordered_map<std::string, SDL_FRect>& contentRects,
                    ChannelRegistry& channels) noexcept;

  const std::string& activeChannel() const noexcept { return mActiveChannel; }

private:
  void invalidate() noexcept { mInvalidated.store(true, std::memory_order_release); }

  std::atomic_bool& mInvalidated;
  bool mDragging = false;
  int mLastX = 0;
  int mLastY = 0;
  std::string mActiveChannel;
  float mZoomMin = 0.05f;
  float mZoomMax = 32.0f;
};

} // namespace Ravl2::DebugDisplay
