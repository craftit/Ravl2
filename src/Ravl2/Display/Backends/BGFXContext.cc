#include "Ravl2/Display/Backends/BGFXContext.hh"
#include <spdlog/spdlog.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#endif

#if defined(RAVL2_WITH_BGFX)
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#endif

namespace Ravl2::DebugDisplay {

#if defined(RAVL2_WITH_BGFX)

// bgfx callback to capture internal logs and errors
class BgfxCallback : public bgfx::CallbackI {
public:
  void fatal(const char* /*filePath*/, uint16_t /*line*/, bgfx::Fatal::Enum code, const char* str) override {
    SPDLOG_CRITICAL("bgfx fatal error {}: {}", static_cast<int>(code), str);
  }
  
  void traceVargs(const char* /*filePath*/, uint16_t /*line*/, const char* format, va_list argList) override {
    char buffer[2048];
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
    vsnprintf(buffer, sizeof(buffer), format, argList);
    #pragma clang diagnostic pop
    SPDLOG_DEBUG("bgfx trace: {}", buffer);
  }
  
  void profilerBegin(const char* /*name*/, uint32_t /*abgr*/, const char* /*filePath*/, uint16_t /*line*/) override {}
  void profilerBeginLiteral(const char* /*name*/, uint32_t /*abgr*/, const char* /*filePath*/, uint16_t /*line*/) override {}
  void profilerEnd() override {}
  
  uint32_t cacheReadSize(uint64_t /*id*/) override { return 0; }
  bool cacheRead(uint64_t /*id*/, void* /*data*/, uint32_t /*size*/) override { return false; }
  void cacheWrite(uint64_t /*id*/, const void* /*data*/, uint32_t /*size*/) override {}
  
  void screenShot(const char* /*filePath*/, uint32_t /*width*/, uint32_t /*height*/, uint32_t /*pitch*/, 
                  const void* /*data*/, uint32_t /*size*/, bool /*yflip*/) override {}
  
  void captureBegin(uint32_t /*width*/, uint32_t /*height*/, uint32_t /*pitch*/, bgfx::TextureFormat::Enum /*format*/, bool /*yflip*/) override {}
  void captureEnd() override {}
  void captureFrame(const void* /*data*/, uint32_t /*size*/) override {}
};

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

  static BgfxCallback s_callback;
  
  bgfx::Init init{};
  init.type = toBgfxType(params.backend);
  init.vendorId = BGFX_PCI_ID_NONE;
  init.resolution.width = static_cast<uint32_t>(params.width);
  init.resolution.height = static_cast<uint32_t>(params.height);
  init.resolution.reset = BGFX_RESET_VSYNC;
  init.callback = &s_callback;
  
#ifdef __APPLE__
  // On macOS, Metal requires main thread access; disable bgfx render thread
  init.resolution.reset |= BGFX_RESET_NONE;
  // Note: This doesn't actually disable threading; bgfx needs to be compiled with BGFX_CONFIG_MULTITHREADED=0
#endif
  
  // Set platform data directly in init structure (not via global setPlatformData)
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
  init.platformData.nwh = wmi.info.win.window;
#elif defined(SDL_VIDEO_DRIVER_COCOA)
  // On macOS with Metal backend, bgfx needs the NSView's CAMetalLayer, not the NSWindow
  // SDL_WINDOW_METAL flag ensures the view has a Metal layer
  NSWindow* nsWindow = wmi.info.cocoa.window;
  NSView* contentView = [nsWindow contentView];
  // __bridge is an Objective-C ARC keyword, not a C++ cast; disable old-style-cast warning
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wold-style-cast"
  init.platformData.nwh = (__bridge void*)contentView;
  #pragma clang diagnostic pop
#elif defined(SDL_VIDEO_DRIVER_X11)
  init.platformData.ndt = wmi.info.x11.display;
  init.platformData.nwh = (void*)(uintptr_t)wmi.info.x11.window;
#elif defined(SDL_VIDEO_DRIVER_WAYLAND)
  init.platformData.ndt = wmi.info.wl.display;
  init.platformData.nwh = wmi.info.wl.surface;
#elif defined(__ANDROID__)
  init.platformData.nwh = wmi.info.android.window;
#else
  init.platformData.nwh = nullptr;
#endif

  SPDLOG_INFO("BGFXContext: attempting init with backend={}, size={}x{}, nwh={}", 
               backendName(params.backend), params.width, params.height, init.platformData.nwh);

  if (!bgfx::init(init)) {
    SPDLOG_ERROR("BGFXContext: bgfx::init failed ({}x{}, backend={})", params.width, params.height, backendName(params.backend));
    SPDLOG_ERROR("BGFXContext: renderer type requested: {}", static_cast<int>(init.type));
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
