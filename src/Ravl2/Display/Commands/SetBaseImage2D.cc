#include "Ravl2/Display/Commands/SetBaseImage2D.hh"
#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/Image2DNode.hh"
#include <spdlog/spdlog.h>

namespace Ravl2::DebugDisplay {

void SetBaseImage2D::apply(ChannelRegistry &channels) {
  auto &ch = channels.getOrCreateChannel(channel);
  if (!ch.baseImage2D) {
    ch.baseImage2D = std::make_unique<Image2DNode>();
  }
  auto *node = static_cast<Image2DNode*>(ch.baseImage2D.get());
  if (isFloat) {
    if (!f32.empty()) {
      node->setFromF32(f32.data(), width, height);
      SPDLOG_INFO("DebugDisplay: Updated float image {}x{} on channel '{}'", width, height, channel);
    }
  } else {
    if (!u8.empty()) {
      node->setFromU8(u8.data(), width, height);
      SPDLOG_INFO("DebugDisplay: Updated u8 image {}x{} on channel '{}'", width, height, channel);
    }
  }
  // View (pan/zoom) is owned by the ChannelState (view2D); no sync into nodes.
}

} // namespace Ravl2::DebugDisplay
