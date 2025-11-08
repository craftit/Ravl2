#pragma once

#include <string>
#include <optional>

namespace Ravl2::DebugDisplay {

//! Minimal placeholder for bgfx platform/context integration.
//! Real implementation will initialize bgfx with SDL native window handle,
//! select backend (Vulkan default with fallbacks), and manage resize.
struct BGFXContext {
  enum class Backend {
    Auto,
    Vulkan,
    Metal,
    D3D12,
    D3D11,
    OpenGL
  };

  struct InitParams {
    Backend backend = Backend::Vulkan;
    int width = 1280;
    int height = 720;
    void* nativeWindow = nullptr; //! platform-specific window handle
  };

  bool initialized() const noexcept { return m_initialized; }

  // Stubbed init; returns false until bgfx wiring is added.
  bool init(const InitParams& params) noexcept;

  // To be called on window resize.
  void resize(int width, int height) noexcept;

  // Frame submission boundary.
  void frame() noexcept;

  // Shutdown bgfx.
  void shutdown() noexcept;

  // Human-readable backend name (stubbed for now).
  static const char* backendName(Backend b) noexcept;

private:
  bool m_initialized = false;
  InitParams m_params{};
};

} // namespace Ravl2::DebugDisplay
