#include "Ravl2/Display/AddPolylineOverlay2D.hh"

#include <utility>

#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/CompositeNode.hh"
#include "Ravl2/Display/Polyline2DNode.hh"
#include "Ravl2/Display/Image2DNodeBase.hh"
#include <spdlog/spdlog.h>

namespace Ravl2::DebugDisplay
{

  static inline std::vector<SDL_FPoint> toFPoints(const Ravl2::PolyLine<float, 2> &poly)
  {
    std::vector<SDL_FPoint> out;
    out.reserve(poly.size());
    for(const auto &p : poly) {
      out.push_back(SDL_FPoint {p[0], p[1]});
    }
    return out;
  }

  void AddPolylineOverlay2D::apply(ChannelRegistry &channels)
  {
    auto &ch = channels.getOrCreateChannel(channel);

    // Create polyline node
    auto polyNode = std::make_unique<Polyline2DNode>();
    polyNode->setVertices(toFPoints(poly));
    polyNode->setColor(rgba);
    polyNode->setThickness(widthPx);
    polyNode->setClosed(closed);

    // If no scene content, just set the polyline (unusual but valid)
    if(!ch.sceneContent) {
      ch.sceneContent = std::move(polyNode);
      SPDLOG_INFO("DebugDisplay: added polyline overlay as sole content on channel '{}'", channel);
      return;
    }

    // If scene content is already a CompositeNode, add to it or replace based on mode
    if(auto *composite = dynamic_cast<CompositeNode *>(ch.sceneContent.get())) {
      if(mode == Mode::Replace) {
        // Keep only the base image (first child), remove all overlays
        if(composite->childCount() > 1) {
          auto baseImage = std::move(composite->children()[0]);
          composite->clear();
          composite->addChild(std::move(baseImage));
        }
      }
      composite->addChild(std::move(polyNode));
      SPDLOG_INFO("DebugDisplay: added polyline overlay to existing composite on channel '{}'", channel);
      return;
    }

    // Scene content exists but is not composite - wrap it in CompositeNode
    auto composite = std::make_unique<CompositeNode>();
    composite->addChild(std::move(ch.sceneContent));// Base image becomes first child
    composite->addChild(std::move(polyNode));       // Overlay becomes second child
    ch.sceneContent = std::move(composite);
    SPDLOG_INFO("DebugDisplay: created composite node with base image + polyline overlay on channel '{}'", channel);
  }

  void ClearOverlays2D::apply(ChannelRegistry &channels)
  {
    auto &ch = channels.getOrCreateChannel(channel);

    // If scene content is a CompositeNode, remove all children except the first (base image)
    if(auto *composite = dynamic_cast<CompositeNode *>(ch.sceneContent.get())) {
      if(composite->childCount() > 1) {
        auto baseImage = std::move(composite->children()[0]);
        ch.sceneContent = std::move(baseImage);
        SPDLOG_INFO("DebugDisplay: cleared overlays for channel '{}', kept base image", channel);
      } else {
        SPDLOG_INFO("DebugDisplay: no overlays to clear for channel '{}'", channel);
      }
    } else {
      SPDLOG_INFO("DebugDisplay: cleared overlays for channel '{}' (no composite)", channel);
    }
  }

}// namespace Ravl2::DebugDisplay
