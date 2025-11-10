#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>

#include <SDL2/SDL.h>

#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/RenderContext.hh"

namespace Ravl2::DebugDisplay::Ui::ChannelWindows {

//! Build per-channel image windows and record last screen-space rects for hit testing.
//! Requires ImGui + bgfx path for rendering images via ImGui::Image helper.
void build(uint16_t fbw, uint16_t fbh,
           ChannelRegistry& channels,
           std::unordered_map<std::string, SDL_FRect>& lastRects);

} // namespace Ravl2::DebugDisplay::Ui::ChannelWindows
