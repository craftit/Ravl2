#include "Ravl2/Display/Commands/SetPointCloud3D.hh"

#include <spdlog/spdlog.h>

#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/Viewport3DNode.hh"

namespace Ravl2::DebugDisplay {

void SetPointCloud3D::apply(ChannelRegistry &channels) {
  // Ensure channel exists
  auto &ch = channels.getOrCreateChannel(channel);
  // Switch to 3D view for 3D payloads by default
  ch.viewMode = ViewMode::View3D;
  ch.flags.wantsFocus3D = true; // request focus on the 3D window next frame
  // Ensure a 3D viewport node exists
  if (!ch.viewport3D) {
    ch.viewport3D = std::make_unique<Viewport3DNode>();
  }
  auto *vp3d = static_cast<Viewport3DNode*>(ch.viewport3D.get());
  if (vp3d) {
    vp3d->lastPointCount = positions.size();
  }
  SPDLOG_INFO("DebugDisplay: Applied SetPointCloud3D with {} points on channel '{}'", positions.size(), channel);
}

} // namespace Ravl2::DebugDisplay
