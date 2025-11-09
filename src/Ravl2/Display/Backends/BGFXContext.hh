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
    Backend backend = Backend::Auto;
    int width = 1280;
    int height = 720;
    void* nativeWindow = nullptr; //! SDL_Window* pointer
  };

  bool initialized() const noexcept { return m_initialized; }

  // Initialize bgfx with the given native SDL window and size.
  bool init(const InitParams& params) noexcept;

  // To be called on window resize.
  void resize(int width, int height) noexcept;

  // Frame submission boundary.
  void frame() noexcept;

  // Shutdown bgfx.
  void shutdown() noexcept;

  // Human-readable backend name.
  static const char* backendName(Backend b) noexcept;

  // Currently selected backend (meaningful only if initialized()).
  Backend backend() const noexcept { return m_params.backend; }

private:
  bool m_initialized = false;
  InitParams m_params{};
};

} // namespace Ravl2::DebugDisplay
