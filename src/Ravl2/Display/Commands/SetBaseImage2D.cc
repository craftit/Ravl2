#include "Ravl2/Display/Commands/SetBaseImage2D.hh"
#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/Image2DNode.hh"
#include <spdlog/spdlog.h>

namespace Ravl2::DebugDisplay {

void SetBaseImage2D::apply(ChannelRegistry &channels) {
  auto &ch = channels.getOrCreateChannel(channel);

  if (isFloat) {
    // Get or create Image2DNode<float>
    Image2DNode<float>* node = nullptr;
    if (auto* img = dynamic_cast<Image2DNode<float>*>(ch.sceneContent.get())) {
      node = img;
    } else {
      // Create new Image2DNode<float> as the scene content
      auto newNode = std::make_unique<Image2DNode<float>>();
      node = newNode.get();
      ch.sceneContent = std::move(newNode);
    }

    if (!f32.empty()) {
      node->setFromF32(f32.data(), width, height);
      SPDLOG_INFO("DebugDisplay: Updated float image {}x{} on channel '{}'", width, height, channel);
    }
  } else {
    // Get or create Image2DNode<uint8_t>
    Image2DNode<uint8_t>* node = nullptr;
    if (auto* img = dynamic_cast<Image2DNode<uint8_t>*>(ch.sceneContent.get())) {
      node = img;
    } else {
      // Create new Image2DNode<uint8_t> as the scene content
      auto newNode = std::make_unique<Image2DNode<uint8_t>>();
      node = newNode.get();
      ch.sceneContent = std::move(newNode);
    }

    if (!u8.empty()) {
      node->setFromU8(u8.data(), width, height);
      SPDLOG_INFO("DebugDisplay: Updated u8 image {}x{} on channel '{}'", width, height, channel);
    }
  }
  // View (pan/zoom) is owned by the ChannelState (view2D); no sync into nodes.
}

} // namespace Ravl2::DebugDisplay
