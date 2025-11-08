#include "Ravl2/Display/Commands/SetNormalization2D.hh"
#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/Image2DNode.hh"
#include <spdlog/spdlog.h>

namespace Ravl2::DebugDisplay {

void SetNormalization2D::apply(ChannelRegistry &channels) {
  auto &ch = channels.getOrCreateChannel(channel);
  ch.norm = settings;
  if (ch.baseImage2D) {
    // Keep node's local settings in sync for sampling paths that consult the node.
    if (auto *node = dynamic_cast<Image2DNode*>(ch.baseImage2D.get())) {
      node->norm = settings;
    }
  }
  SPDLOG_INFO("DebugDisplay: Set normalization on '{}' to policy={} (fixed: [{},{}], pct: [{},{}])",
              channel,
              static_cast<int>(settings.policy),
              settings.minVal, settings.maxVal,
              settings.lowPct, settings.highPct);
}

} // namespace Ravl2::DebugDisplay
