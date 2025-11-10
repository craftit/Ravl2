#pragma once

#include <cstdint>
#include <spdlog/spdlog.h>

namespace Ravl2::DebugDisplay {

//! Thin wrapper that encapsulates Dear ImGui integration over bgfx.
//! Keeps BGFXContext focused on renderer lifecycle and isolates ImGui glue.
struct ImguiBgfxBridge {
  bool init(float fontSize = 18.0f) noexcept;
  void shutdown() noexcept;

  void beginFrame(int mouseX,
                  int mouseY,
                  uint8_t mouseButtons,
                  int32_t scroll,
                  uint16_t fbWidth,
                  uint16_t fbHeight) noexcept;

  void endFrame() noexcept;

  bool initialized() const noexcept { return mInitialized; }

private:
  bool mInitialized = false;
};

} // namespace Ravl2::DebugDisplay
