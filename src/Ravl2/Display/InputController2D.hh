#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <unordered_map>

#include <SDL2/SDL.h>

#include "Ravl2/Display/Channel.hh"

namespace Ravl2::DebugDisplay
{

  //! InputController2D
  //! @brief Simple per-session input controller for 2D pan/zoom.
  //! @details Owns drag state and active channel selection; stateless channel data lives in
  //! ChannelRegistry. Uses screen-space mouse coordinates and channel image/content rects computed
  //! by the UI to decide when to start/continue pan and how to anchor zoom around the cursor.
  //! @threadsafe No. Call only from the GUI thread.
  class InputController2D
  {
  public:
    //! @param invalidated Reference to the global invalidation flag to request redraws.
    //! @param zoomMin Minimum allowed zoom factor (applied symmetrically to X/Y).
    //! @param zoomMax Maximum allowed zoom factor (applied symmetrically to X/Y).
    InputController2D(std::atomic_bool &invalidated,
                      float zoomMin,
                      float zoomMax) noexcept
        : mInvalidated(invalidated), mZoomMin(zoomMin), mZoomMax(zoomMax) {}

    //! Update zoom limits at runtime.
    void setZoomLimits(float minVal, float maxVal) noexcept
    {
      mZoomMin = minVal;
      mZoomMax = maxVal;
    }

    //! Begin a potential drag if the click is inside both the image rect and the content rect.
    //! Coordinates are in screen space (ImGui).
    void onMouseButtonDown(int x, int y,
                           const std::unordered_map<std::string, SDL_FRect> &lastRects,
                           const std::unordered_map<std::string, SDL_FRect> &contentRects) noexcept;

    //! Explicit-channel overload to avoid ambiguity with overlapping windows.
    void onMouseButtonDown(int x, int y,
                           const std::string &channelName,
                           const std::unordered_map<std::string, SDL_FRect> &lastRects,
                           const std::unordered_map<std::string, SDL_FRect> &contentRects) noexcept;

    //! End drag for the given SDL button (left button stops panning).
    void onMouseButtonUp(uint8_t sdlButton) noexcept;

    //! Apply panning based on mouse motion since last event.
    void onMouseMotion(int x, int y,
                       ChannelRegistry &channels) noexcept;

    //! Zoom around the cursor using content-origin-based mapping; auto-select channel under cursor.
    void onMouseWheel(int wheelY, int mouseX, int mouseY,
                      const std::unordered_map<std::string, SDL_FRect> &lastRects,
                      const std::unordered_map<std::string, SDL_FRect> &contentRects,
                      ChannelRegistry &channels) noexcept;

    //! Explicit-channel overload mirroring the above but constrained to a given channel.
    void onMouseWheel(int wheelY, int mouseX, int mouseY,
                      const std::string &channelName,
                      const std::unordered_map<std::string, SDL_FRect> &lastRects,
                      const std::unordered_map<std::string, SDL_FRect> &contentRects,
                      ChannelRegistry &channels) noexcept;

    //! @return Name of the active channel currently being dragged, or empty if none.
    const std::string &activeChannel() const noexcept { return mActiveChannel; }

  private:
    void invalidate() noexcept { mInvalidated.store(true, std::memory_order_release); }

    std::atomic_bool &mInvalidated;//!< Redraw flag toggled when state changes
    bool mDragging = false;        //!< True while left-button drag is active
    int mLastX = 0;                //!< Last mouse x (screen space)
    int mLastY = 0;                //!< Last mouse y (screen space)
    std::string mActiveChannel;    //!< Channel being dragged (if any)
    float mZoomMin = 0.05f;        //!< Min zoom factor
    float mZoomMax = 32.0f;        //!< Max zoom factor
  };

}// namespace Ravl2::DebugDisplay
