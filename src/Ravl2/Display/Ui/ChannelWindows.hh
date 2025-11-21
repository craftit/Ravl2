#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <atomic>

#include <SDL2/SDL.h>

#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/RenderContext.hh"

namespace Ravl2::DebugDisplay::Ui::ChannelWindows
{

  //! Build per-channel image windows and record last screen-space rects for hit testing.
  //! Also records per-window image origin (screen space of the content cursor before applying translation)
  //! and content rect for precise input gating and zoom anchoring.
  //! Additionally reports which channel window is currently hovered (top-most), if any.
  //! Requires ImGui + bgfx path for rendering images via ImGui::Image helper.
  void build(uint16_t fbw, uint16_t fbh,
             ChannelRegistry &channels,
             std::unordered_map<std::string, SDL_FRect> &lastRects,
             std::unordered_map<std::string, SDL_FPoint> &imageOrigins,
             std::unordered_map<std::string, SDL_FRect> &contentRects,
             std::atomic_bool &invalidated,
             std::string &hoveredChannelOut,
             std::string &hoveredImageChannelOut);

}// namespace Ravl2::DebugDisplay::Ui::ChannelWindows
