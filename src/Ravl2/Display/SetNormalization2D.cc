#include "Ravl2/Display/SetNormalization2D.hh"
#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/Image2DNode.hh"
#include "Ravl2/Display/CompositeNode.hh"
#include <spdlog/spdlog.h>

namespace Ravl2::DebugDisplay
{

  void SetNormalization2D::apply(ChannelRegistry &channels)
  {
    auto &ch = channels.getOrCreateChannel(channel);
    ch.norm = settings;
    // Keep node's local settings in sync for sampling paths that consult the node.
    // Only float images use normalization. The image may sit under overlays in a CompositeNode.
    if(auto *node = dynamic_cast<Image2DNode<float> *>(baseContentSlot(ch.sceneContent).get())) {
      node->setNormalization(settings);
    }
    SPDLOG_INFO("DebugDisplay: Set normalization on '{}' to policy={} (fixed: [{},{}], pct: [{},{}])",
                channel,
                static_cast<int>(settings.policy),
                settings.minVal, settings.maxVal,
                settings.lowPct, settings.highPct);
  }

}// namespace Ravl2::DebugDisplay
