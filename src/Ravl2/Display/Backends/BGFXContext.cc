#include "Ravl2/Display/Backends/BGFXContext.hh"
#include <spdlog/spdlog.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#if defined(RAVL2_WITH_BGFX)
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#endif

namespace Ravl2::DebugDisplay {

#if defined(RAVL2_WITH_BGFX)
static bgfx::RendererType::Enum toBgfxType(BGFXContext::Backend b) noexcept {
  switch (b) {
    case BGFXContext::Backend::Vulkan: return bgfx::RendererType::Vulkan;
    case BGFXContext::Backend::Metal:  return bgfx::RendererType::Metal;
    case BGFXContext::Backend::D3D12:  return bgfx::RendererType::Direct3D12;
    case BGFXContext::Backend::D3D11:  return bgfx::RendererType::Direct3D11;
    case BGFXContext::Backend::OpenGL: return bgfx::RendererType::OpenGL;
    case BGFXContext::Backend::Auto:   return bgfx::RendererType::Count; // auto
  }
  return bgfx::RendererType::Count;
}

bool BGFXContext::init(const InitParams& params) noexcept {
  m_params = params;

  if (m_initialized) return true;
  if (params.nativeWindow == nullptr) {
    SPDLOG_ERROR("BGFXContext: init failed, nativeWindow is null");
    return false;
  }

  SDL_Window* window = static_cast<SDL_Window*>(params.nativeWindow);

  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  if (!SDL_GetWindowWMInfo(window, &wmi)) {
    SPDLOG_ERROR("BGFXContext: SDL_GetWindowWMInfo failed: {}", SDL_GetError());
    return false;
  }

  bgfx::PlatformData pd{};
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
  pd.nwh = wmi.info.win.window;
#elif defined(SDL_VIDEO_DRIVER_COCOA)
  pd.nwh = wmi.info.cocoa.window;
#elif defined(SDL_VIDEO_DRIVER_X11)
  pd.ndt = wmi.info.x11.display;
  pd.nwh = (void*)(uintptr_t)wmi.info.x11.window;
#elif defined(SDL_VIDEO_DRIVER_WAYLAND)
  pd.ndt = wmi.info.wl.display;
  pd.nwh = wmi.info.wl.surface;
#elif defined(__ANDROID__)
  pd.nwh = wmi.info.android.window;
#else
  pd.nwh = nullptr;
#endif
  bgfx::setPlatformData(pd);

  bgfx::Init init{};
  init.type = toBgfxType(params.backend);
  init.vendorId = BGFX_PCI_ID_NONE;
  init.resolution.width = static_cast<uint32_t>(params.width);
  init.resolution.height = static_cast<uint32_t>(params.height);
  init.resolution.reset = BGFX_RESET_VSYNC;

  if (!bgfx::init(init)) {
    SPDLOG_ERROR("BGFXContext: bgfx::init failed ({}x{}, backend={})", params.width, params.height, backendName(params.backend));
    m_initialized = false;
    return false;
  }

  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
  bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(params.width), static_cast<uint16_t>(params.height));

  m_initialized = true;
  SPDLOG_INFO("BGFXContext: initialized ({}x{}, backend={})", params.width, params.height, backendName(params.backend));
  return true;
}

void BGFXContext::resize(int width, int height) noexcept {
  m_params.width = width; m_params.height = height;
  if (!m_initialized) return;
  bgfx::reset(static_cast<uint32_t>(width), static_cast<uint32_t>(height), BGFX_RESET_VSYNC);
  bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(width), static_cast<uint16_t>(height));
  SPDLOG_DEBUG("BGFXContext: resize to {}x{}", width, height);
}

void BGFXContext::frame() noexcept {
  if (!m_initialized) return;
  bgfx::touch(0);
  bgfx::frame();
}

void BGFXContext::shutdown() noexcept {
  if (m_initialized) {
    bgfx::shutdown();
    SPDLOG_INFO("BGFXContext: shutdown");
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

#endif // RAVL2_WITH_BGFX

} // namespace Ravl2::DebugDisplay

#if !defined(RAVL2_WITH_BGFX)

namespace Ravl2::DebugDisplay {

bool BGFXContext::init(const InitParams& params) noexcept {
  m_params = params;
  SPDLOG_INFO("BGFXContext (stub): init skipped; bgfx not available");
  m_initialized = false;
  return false;
}

void BGFXContext::resize(int width, int height) noexcept {
  (void)width; (void)height;
}

void BGFXContext::frame() noexcept {}

void BGFXContext::shutdown() noexcept {}

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

#endif
