#pragma once

#include <string>
#include "Ravl2/Display/IRenderCommand.hh"
#include "Ravl2/Display/Normalization.hh"

namespace Ravl2::DebugDisplay {

//! Command to set normalization/display settings for a 2D view on a channel.
struct SetNormalization2D : public IRenderCommand {
  std::string channel;
  NormalizationSettings settings{};

  explicit SetNormalization2D(std::string ch, NormalizationSettings s)
      : channel(std::move(ch)), settings(s) {}

  void apply(ChannelRegistry &channels) override;
};

} // namespace Ravl2::DebugDisplay
