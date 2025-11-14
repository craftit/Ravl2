#pragma once

#include <memory>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "Ravl2/Display/IRenderCommand.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Display/Image2DNode.hh"

namespace Ravl2::DebugDisplay {

//! Templated render command to set/update the base 2D image for a channel.
//! Each template instantiation handles a specific pixel type (uint8_t, float, PixelRGB8, etc.)
template<typename PixelT>
struct SetBaseImage2D : public IRenderCommand {
  std::string channel;           //!< Target channel name
  int width = 0;
  int height = 0;
  std::vector<PixelT> data;      //!< width*height pixel data

  explicit SetBaseImage2D(std::string ch) : channel(std::move(ch)) {}

  void apply(ChannelRegistry &channels) override {
    auto &ch = channels.getOrCreateChannel(channel);

    // Get or create Image2DNode<PixelT>
    Image2DNode<PixelT>* node = nullptr;
    if (auto* img = dynamic_cast<Image2DNode<PixelT>*>(ch.sceneContent.get())) {
      node = img;
    } else {
      // Create new Image2DNode<PixelT> as the scene content
      auto newNode = std::make_unique<Image2DNode<PixelT>>();
      node = newNode.get();
      ch.sceneContent = std::move(newNode);
    }

    if (!data.empty()) {
      node->setData(data.data(), width, height);
      SPDLOG_INFO("DebugDisplay: Updated image {}x{} on channel '{}'", width, height, channel);
    }
  }
};

// Common type aliases for convenience
using SetBaseImage2D_U8 = SetBaseImage2D<uint8_t>;
using SetBaseImage2D_F32 = SetBaseImage2D<float>;
using SetBaseImage2D_RGB8 = SetBaseImage2D<PixelRGB8>;
using SetBaseImage2D_I16 = SetBaseImage2D<int16_t>;
using SetBaseImage2D_I32 = SetBaseImage2D<int32_t>;

} // namespace Ravl2::DebugDisplay
