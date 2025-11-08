#pragma once

#include <cstdint>

namespace Ravl2::DebugDisplay {

// Placeholder render context for future bgfx/ImGui integration.
// Owns no resources; passed to nodes during prepare/render when Phase 2 wires rendering.
struct RenderContext {
  int framebufferWidth = 0;
  int framebufferHeight = 0;
};

} // namespace Ravl2::DebugDisplay
