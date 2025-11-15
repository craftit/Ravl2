#pragma once

#include <vector>
#include <cstdint>
#include <SDL2/SDL.h>

#include "Ravl2/Display/ISceneNode.hh"
#include "Ravl2/Geometry/ScaleTranslate.hh"

// Forward declare ImGui draw list
struct ImDrawList;

namespace Ravl2::DebugDisplay {

//! 2D polyline overlay as an ISceneNode.
//! Renders a polyline in image coordinates, transformed to screen space via the view2D transform.
class Polyline2DNode : public ISceneNode {
private:
  std::vector<SDL_FPoint> m_vertices;  //!< Vertices in image-space coordinates
  uint32_t m_rgba = 0xff00ff00u;       //!< Line color (default green, RGBA packed)
  float m_thickness = 1.0f;            //!< Line thickness in screen pixels
  bool m_closed = false;               //!< Whether to draw closing segment

public:
  Polyline2DNode() = default;
  ~Polyline2DNode() override = default;

  // Disable copy, allow move
  Polyline2DNode(const Polyline2DNode&) = delete;
  Polyline2DNode& operator=(const Polyline2DNode&) = delete;
  Polyline2DNode(Polyline2DNode&&) = default;
  Polyline2DNode& operator=(Polyline2DNode&&) = default;

  //! Set vertices in image-space coordinates
  void setVertices(std::vector<SDL_FPoint> vertices) { m_vertices = std::move(vertices); }
  void setColor(uint32_t rgba) { m_rgba = rgba; }
  void setThickness(float thickness) { m_thickness = thickness; }
  void setClosed(bool closed) { m_closed = closed; }

  const std::vector<SDL_FPoint>& vertices() const { return m_vertices; }
  uint32_t color() const { return m_rgba; }
  float thickness() const { return m_thickness; }
  bool closed() const { return m_closed; }

  //! ISceneNode interface: no GPU preparation needed for overlays
  void prepare(RenderContext& ctx) override { (void)ctx; }

  //! ISceneNode interface: render polyline to ImGui draw list
  void render(RenderContext& ctx) override;

  //! Overlays don't support pixel queries
  bool supportsPixelQuery() const override { return false; }

  PixelQueryResult queryPixelInfo(int x, int y) const override {
    (void)x; (void)y;
    return {.valid = false, .coordinateText = "", .valueText = "", .extraInfo = std::nullopt};
  }
};

} // namespace Ravl2::DebugDisplay
