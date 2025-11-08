#include "Ravl2/Display/Backends/BGFXContext.hh"
#include <spdlog/spdlog.h>

namespace Ravl2::DebugDisplay {

bool BGFXContext::init(const InitParams& params) noexcept {
  m_params = params;
  // NOTE: This is a stub. Real implementation will set bgfx::PlatformData from nativeWindow,
  // choose renderer type based on params.backend, and call bgfx::init().
  SPDLOG_INFO("BGFXContext (stub): init requested ({}x{}, backend={})",
              params.width, params.height, backendName(params.backend));
  m_initialized = false; // keep false until bgfx is actually wired
  return m_initialized;
}

void BGFXContext::resize(int width, int height) noexcept {
  m_params.width = width; m_params.height = height;
  // Stub: would call bgfx::reset and update view rects
  SPDLOG_DEBUG("BGFXContext (stub): resize to {}x{}", width, height);
}

void BGFXContext::frame() noexcept {
  // Stub: would call bgfx::frame(); here we do nothing
}

void BGFXContext::shutdown() noexcept {
  if (m_initialized) {
    // Stub: would call bgfx::shutdown(); here we only log
    SPDLOG_INFO("BGFXContext (stub): shutdown");
    m_initialized = false;
  }
}

const char* BGFXContext::backendName(Backend b) noexcept {
  switch (b) {
    case Backend::Auto:   return "Auto";
    case Backend::Vulkan: return "Vulkan";
    case Backend::Metal:  return "Metal";
    case Backend::D3D12:  return "D3D12";
    case Backend::D3D11:  return "D3D11";
    case Backend::OpenGL: return "OpenGL";
  }
  return "Unknown";
}

} // namespace Ravl2::DebugDisplay
