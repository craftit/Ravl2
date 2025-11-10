#pragma once

#include <unordered_map>
#include <string>
#include <SDL2/SDL.h>

#include "Ravl2/Display/Channel.hh"

namespace Ravl2::DebugDisplay::Ui::StatusBar {

//! Build a small status bar window that shows pixel inspector data.
//! Safe to call each frame; displays channel name, (x,y), raw/disp values
//! for the pixel under the mouse when hovering an image.
void build(const std::unordered_map<std::string, SDL_FRect>& lastRects,
           ChannelRegistry& channels);

} // namespace Ravl2::DebugDisplay::Ui::StatusBar
