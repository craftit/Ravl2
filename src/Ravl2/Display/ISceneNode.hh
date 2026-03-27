#pragma once

#include <optional>
#include <string>

namespace Ravl2::DebugDisplay
{

  struct RenderContext;// fwd decl for future bgfx/imgui context

  //! Result from querying pixel information at a specific coordinate.
  struct PixelQueryResult {
    bool valid = false;                  //!< Hit test success
    std::string coordinateText;          //!< "x: 123, y: 456"
    std::string valueText;               //!< Type-specific formatted value
    std::optional<std::string> extraInfo;//!< Optional additional details
  };

  //! Persistent scene node (owns GPU resources; lives inside a channel state).
  struct ISceneNode {
    virtual ~ISceneNode() = default;
    virtual void prepare(RenderContext &) {}
    virtual void render(RenderContext &) {}

    //! Returns true if this node supports pixel queries (e.g., 2D images).
    //! Default implementation returns false (3D nodes, overlays, etc.).
    virtual bool supportsPixelQuery() const { return false; }

    //! Query pixel information at image coordinates (x, y).
    //! Returns valid=false if query is not supported or coordinates are out of bounds.
    virtual PixelQueryResult queryPixelInfo([[maybe_unused]] int x, [[maybe_unused]] int y) const
    {
      return {.valid = false, .coordinateText = "", .valueText = "", .extraInfo = std::nullopt};
    }
  };

}// namespace Ravl2::DebugDisplay
