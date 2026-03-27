#pragma once

#include <cstdint>
#include <expected>
#include <spdlog/spdlog.h>

namespace Ravl2::DebugDisplay
{

  //! ImguiBgfxBridge
  //! @brief Thin wrapper that encapsulates Dear ImGui integration over bgfx.
  //! @details Separates ImGui+bgfx glue from the renderer context so `BGFXContext` can
  //! remain single-responsibility. Provides frame begin/end and lifetime management.
  //! @threadsafe No. Call from the GUI thread only.
  struct ImguiBgfxBridge {
    //! Initialize ImGui (bgfx backend).
    //! @param fontSize Default font size in pixels.
    //! @return true on success. Prefer `initEx` to avoid internal logging.
    bool init(float fontSize = 18.0f) noexcept;

    //! Initialize ImGui (bgfx backend) with error reporting and no internal logging.
    //! @param fontSize Default font size in pixels.
    //! @return `std::expected<void, std::string>` with error message on failure.
    std::expected<void, std::string> initEx(float fontSize = 18.0f) noexcept;

    //! Shutdown and release ImGui (bgfx backend) resources.
    void shutdown() noexcept;

    //! Begin a new ImGui frame providing mouse input and framebuffer size.
    //! @param mouseX,mouseY Mouse position in window coordinates (screen space).
    //! @param mouseButtons Bitmask matching ImGui backend expectations.
    //! @param scroll Accumulated scroll since last frame.
    //! @param fbWidth,fbHeight Current framebuffer size in pixels.
    void beginFrame(int mouseX,
                    int mouseY,
                    uint8_t mouseButtons,
                    int32_t scroll,
                    uint16_t fbWidth,
                    uint16_t fbHeight) noexcept;

    //! End and render the current ImGui frame.
    void endFrame() noexcept;

    //! @return true if initialized.
    bool initialized() const noexcept { return mInitialized; }

  private:
    bool mInitialized = false;//!< Internal init state
  };

}// namespace Ravl2::DebugDisplay
