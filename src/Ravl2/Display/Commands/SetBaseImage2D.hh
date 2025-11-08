#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Ravl2/Display/IRenderCommand.hh"

namespace Ravl2::DebugDisplay {

//! Render command to set/update the base 2D image for a channel.
struct SetBaseImage2D : public IRenderCommand {
  std::string channel;      //!< Target channel name
  int width = 0;
  int height = 0;
  bool isFloat = false;     //!< false = U8, true = F32
  std::vector<uint8_t> u8;  //!< width*height
  std::vector<float> f32;   //!< width*height

  explicit SetBaseImage2D(std::string ch) : channel(std::move(ch)) {}

  void apply(ChannelRegistry &channels) override;
};

} // namespace Ravl2::DebugDisplay
