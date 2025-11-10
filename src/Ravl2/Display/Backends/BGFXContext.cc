#include "Ravl2/Display/Backends/BGFXContext.hh"
#include <spdlog/spdlog.h>
#include <atomic>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#ifdef __APPLE__
#include <objc/message.h>
#include <objc/runtime.h>
#endif

#if defined(RAVL2_WITH_BGFX)
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#endif

namespace Ravl2::DebugDisplay {

#if defined(RAVL2_WITH_BGFX)

// Global flag to detect bgfx fatals during initialization attempts
static std::atomic_bool s_bgfxFatalSeen{false};

// bgfx callback to capture internal logs and errors
class BgfxCallback : public bgfx::CallbackI {
public:
  void fatal(const char* /*filePath*/, uint16_t /*line*/, bgfx::Fatal::Enum code, const char* str) override {
    SPDLOG_CRITICAL("bgfx fatal error {}: {}", static_cast<int>(code), str);
    // Mark that a fatal occurred so the current init attempt can be treated as failed
    s_bgfxFatalSeen.store(true, std::memory_order_release);
  }
  
  void traceVargs(const char* /*filePath*/, uint16_t /*line*/, const char* format, va_list argList) override {
    char buffer[2048];
#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
    vsnprintf(buffer, sizeof(buffer), format, argList);
#ifdef __clang__
    #pragma clang diagnostic pop
#endif
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
  const char* sdlDriver = SDL_GetCurrentVideoDriver();
  SPDLOG_INFO("BGFXContext: SDL video driver='{}', WM subsystem={}", (sdlDriver ? sdlDriver : "(null)"), static_cast<int>(wmi.subsystem));

  static BgfxCallback s_callback;
  
  auto buildInit = [&](bgfx::RendererType::Enum rt) {
    bgfx::Init init{};
    init.type = rt; // bgfx::RendererType::Count means auto
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
    // On macOS with Metal backend, bgfx expects a CAMetalLayer* in platformData.nwh.
    // Use Objective-C runtime to avoid requiring Objective-C++ compilation.
    void* nsWindowVoid = wmi.info.cocoa.window;
    id nsWindow = static_cast<id>(nsWindowVoid);
    SEL selContentView = sel_registerName("contentView");
    id contentView = reinterpret_cast<id (*)(id, SEL)>(objc_msgSend)(nsWindow, selContentView);
    SEL selWantsLayer = sel_registerName("wantsLayer");
    bool wantsLayer = reinterpret_cast<bool (*)(id, SEL)>(objc_msgSend)(contentView, selWantsLayer);
    if (!wantsLayer) {
      SEL selSetWantsLayer = sel_registerName("setWantsLayer:");
      reinterpret_cast<void (*)(id, SEL, bool)>(objc_msgSend)(contentView, selSetWantsLayer, true);
    }
    SEL selLayer = sel_registerName("layer");
    id layer = reinterpret_cast<id (*)(id, SEL)>(objc_msgSend)(contentView, selLayer);
    Class CAMetalLayerClass = objc_getClass("CAMetalLayer");
    SEL selIsKindOfClass = sel_registerName("isKindOfClass:");
    bool isMetalLayer = (layer != nil) ? reinterpret_cast<bool (*)(id, SEL, Class)>(objc_msgSend)(layer, selIsKindOfClass, CAMetalLayerClass) : false;
    if (!isMetalLayer) {
      SEL selLayerClassMethod = sel_registerName("layer");
      layer = reinterpret_cast<id (*)(Class, SEL)>(objc_msgSend)(CAMetalLayerClass, selLayerClassMethod);
      SEL selSetLayer = sel_registerName("setLayer:");
      reinterpret_cast<void (*)(id, SEL, id)>(objc_msgSend)(contentView, selSetLayer, layer);
    }
    // Set pixelFormat = MTLPixelFormatBGRA8Unorm (80) if available
    SEL selSetPixelFormat = sel_registerName("setPixelFormat:");
    reinterpret_cast<void (*)(id, SEL, unsigned long)>(objc_msgSend)(layer, selSetPixelFormat, static_cast<unsigned long>(80));
    init.platformData.nwh = static_cast<void *>(layer);
  #elif defined(SDL_VIDEO_DRIVER_X11)
    init.platformData.ndt = wmi.info.x11.display;
    init.platformData.nwh = reinterpret_cast<void *>(wmi.info.x11.window);
  #elif defined(SDL_VIDEO_DRIVER_WAYLAND)
    init.platformData.ndt = wmi.info.wl.display;
    init.platformData.nwh = wmi.info.wl.surface;
  #elif defined(__ANDROID__)
    init.platformData.nwh = wmi.info.android.window;
  #else
    init.platformData.nwh = nullptr;
  #endif
    return init;
  };

#ifdef __APPLE__
  // Force single-threaded mode on macOS to avoid render thread deadlock during init
  // This must be called before bgfx::init()
  bgfx::renderFrame();
#endif

  // Build backend candidate list
  std::vector<Backend> candidates;
  if (params.backend == Backend::Auto) {
  #if defined(_WIN32)
    candidates = { Backend::D3D12, Backend::D3D11, Backend::OpenGL };
  #elif defined(__APPLE__)
    candidates = { Backend::Metal };
  #else
    // Linux: prefer Vulkan first on NVIDIA/X11 setups; OpenGL as fallback
    candidates = { Backend::Vulkan, Backend::OpenGL };
  #endif
  } else {
    candidates = { params.backend };
  }

  for (auto b : candidates) {
    auto rt = toBgfxType(b);
    auto init = buildInit(rt);
    SPDLOG_INFO("BGFXContext: attempting init with backend={}, size={}x{}, nwh={}", backendName(b), params.width, params.height, init.platformData.nwh);
    // Reset fatal flag for this attempt
    s_bgfxFatalSeen.store(false, std::memory_order_release);
    if (bgfx::init(init)) {
      // If bgfx reported a fatal during init, treat as failure and clean up
      if (s_bgfxFatalSeen.load(std::memory_order_acquire)) {
        SPDLOG_WARN("BGFXContext: bgfx::init returned true but a fatal was reported; treating as failure for backend {}", backendName(b));
        bgfx::shutdown();
      } else {
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(params.width), static_cast<uint16_t>(params.height));
        // Enable debug text so HUD can render when desired
        bgfx::setDebug(BGFX_DEBUG_TEXT);
        m_initialized = true;
        m_params.backend = b;
        SPDLOG_INFO("BGFXContext: initialized ({}x{}, backend={})", params.width, params.height, backendName(b));
        return true;
      }
    }
    SPDLOG_WARN("BGFXContext: bgfx::init failed for backend {}", backendName(b));
  }

  // As a last resort, try bgfx auto selection
  {
    auto init = buildInit(bgfx::RendererType::Count);
    SPDLOG_INFO("BGFXContext: attempting init with backend=Auto (bgfx), size={}x{}", params.width, params.height);
    s_bgfxFatalSeen.store(false, std::memory_order_release);
    if (bgfx::init(init)) {
      if (s_bgfxFatalSeen.load(std::memory_order_acquire)) {
        SPDLOG_WARN("BGFXContext: bgfx::init (Auto) returned true but a fatal was reported; treating as failure");
        bgfx::shutdown();
      } else {
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(params.width), static_cast<uint16_t>(params.height));
        m_initialized = true;
        SPDLOG_INFO("BGFXContext: initialized via bgfx Auto ({}x{})", params.width, params.height);
        return true;
      }
    }
  }

  SPDLOG_ERROR("BGFXContext: failed to initialize any backend");
  m_initialized = false;
  return false;
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
    // Disable debug output to avoid driver debug group calls during teardown
    bgfx::setDebug(BGFX_DEBUG_NONE);
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
