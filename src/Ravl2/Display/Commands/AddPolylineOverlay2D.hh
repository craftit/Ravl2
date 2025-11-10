#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

#include "Ravl2/Display/IRenderCommand.hh"
#include "Ravl2/Geometry/PolyLine.hh"

namespace Ravl2::DebugDisplay {

//! Render command to add a 2D polyline overlay to a channel.
struct AddPolylineOverlay2D : public IRenderCommand {
  enum class Mode { Append, Replace };

  std::string channel;
  Ravl2::PolyLine<float,2> poly;  //!< Image-space vertices
  bool closed = false;             //!< Draw closing segment if true
  uint32_t rgba = 0xff00ffffu;     //!< Default magenta (RGBA packed as ImU32)
  float widthPx = 1.5f;            //!< Line width in screen pixels
  Mode mode = Mode::Append;

  AddPolylineOverlay2D() = default;
  explicit AddPolylineOverlay2D(Ravl2::PolyLine<float,2> p) : poly(std::move(p)) {}

  void apply(ChannelRegistry &channels) override;
};

//! Render command to clear all overlays from a channel (no other state changed).
struct ClearOverlays2D : public IRenderCommand {
  std::string channel;
  explicit ClearOverlays2D(std::string ch) : channel(std::move(ch)) {}
  void apply(ChannelRegistry &channels) override;
};

} // namespace Ravl2::DebugDisplay
