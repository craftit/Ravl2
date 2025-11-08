#pragma once

namespace Ravl2::DebugDisplay {

struct RenderContext; // fwd decl for future bgfx/imgui context

//! Persistent scene node (owns GPU resources; lives inside a channel state).
struct ISceneNode {
  virtual ~ISceneNode() = default;
  virtual void prepare(RenderContext &) {}
  virtual void render(RenderContext &) {}
};

} // namespace Ravl2::DebugDisplay
