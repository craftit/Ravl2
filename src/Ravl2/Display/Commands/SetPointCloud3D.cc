#include "Ravl2/Display/Commands/SetPointCloud3D.hh"

#include <spdlog/spdlog.h>

#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/Viewport3DNode.hh"

namespace Ravl2::DebugDisplay
{

  void SetPointCloud3D::apply(ChannelRegistry &channels)
  {
    // Ensure channel exists
    auto &ch = channels.getOrCreateChannel(channel);
    // Switch to 3D view for 3D payloads by default
    ch.viewMode = ViewMode::View3D;
    ch.flags.wantsFocus3D = true;// request focus on the 3D window next frame

    // Get or create the Viewport3DNode
    Viewport3DNode *vp3d = nullptr;
    if(auto *vp = dynamic_cast<Viewport3DNode *>(ch.sceneContent.get())) {
      vp3d = vp;
    } else {
      // Create new Viewport3DNode as the scene content
      auto newVp = std::make_unique<Viewport3DNode>();
      vp3d = newVp.get();
      ch.sceneContent = std::move(newVp);
    }

    if(vp3d) {
      vp3d->lastPointCount = positions.size();
    }
    SPDLOG_INFO("DebugDisplay: Applied SetPointCloud3D with {} points on channel '{}'", positions.size(), channel);
  }

}// namespace Ravl2::DebugDisplay
