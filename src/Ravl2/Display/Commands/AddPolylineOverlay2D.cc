#include "Ravl2/Display/Commands/AddPolylineOverlay2D.hh"

#include <utility>

#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/Overlays/OverlayRenderer2D.hh"
#include <spdlog/spdlog.h>

namespace Ravl2::DebugDisplay {

static inline std::vector<SDL_FPoint> toFPoints(const Ravl2::PolyLine<float,2>& poly) {
  std::vector<SDL_FPoint> out;
  out.reserve(poly.size());
  for (const auto& p : poly) {
    out.push_back(SDL_FPoint{p[0], p[1]});
  }
  return out;
}

void AddPolylineOverlay2D::apply(ChannelRegistry &channels) {
  auto &ch = channels.getOrCreateChannel(channel);
  // TODO Phase 4: Migrate overlays to ISceneNode architecture
  // For now, overlays are temporarily disabled during Phase 0 refactoring
  (void)rgba;
  (void)widthPx;
  (void)mode;
  SPDLOG_WARN("DebugDisplay: overlay polyline not yet supported in unified scene graph (Phase 0) - channel '{}'", ch.name);
}

void ClearOverlays2D::apply(ChannelRegistry &channels) {
  auto &ch = channels.getOrCreateChannel(channel);
  // TODO Phase 4: Migrate overlays to ISceneNode architecture
  SPDLOG_INFO("DebugDisplay: cleared overlays for channel '{}' (no-op in Phase 0)", ch.name);
}

} // namespace Ravl2::DebugDisplay
