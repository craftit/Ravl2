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
  using namespace Ravl2::DebugDisplay::Overlays;
  auto ov = std::make_shared<Lines2DOverlay>();
  ov->vertices = toFPoints(poly);
  ov->rgba = rgba;
  ov->thickness = widthPx;
  ov->closed = closed;
  if (mode == Mode::Replace) {
    ch.overlays.clear();
  }
  ch.overlays.push_back(std::move(ov));
  SPDLOG_INFO("DebugDisplay: overlay polyline applied to '{}' ({} vertices, mode={}, closed={}, width={})",
              ch.name, poly.size(), (mode==Mode::Append?"Append":"Replace"), closed?"true":"false", widthPx);
}

void ClearOverlays2D::apply(ChannelRegistry &channels) {
  auto &ch = channels.getOrCreateChannel(channel);
  ch.overlays.clear();
  SPDLOG_INFO("DebugDisplay: cleared overlays for channel '{}'", ch.name);
}

} // namespace Ravl2::DebugDisplay
