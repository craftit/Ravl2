#include "Ravl2/Display/DebugDisplay.hh"
#include "Ravl2/Display/IRenderCommand.hh"
#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/Image2DNode.hh"
#include "Ravl2/Display/Normalization.hh"

#include "Ravl2/ThreadedQueue.hh"

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>
#include <spdlog/spdlog.h>
#include <thread>
#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdio>
#include "Ravl2/Display/Backends/BGFXContext.hh"
#include "Ravl2/Display/Backends/ImguiBgfxBridge.hh"
#include "Ravl2/Display/Ui/Dockspace.hh"
#include "Ravl2/Display/Ui/ControlsPanel.hh"
#include "Ravl2/Display/Ui/ChannelWindows.hh"
#include "Ravl2/Display/Ui/StatusBar.hh"
#include "Ravl2/Display/Ui/Plots.hh"
#include "Ravl2/Display/InputController2D.hh"
#include "Ravl2/Display/PixelInspector2D.hh"
#if defined(RAVL2_WITH_BGFX)
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#endif

#if defined(RAVL2_WITH_BGFX)
namespace {
  struct NonImGuiBgfxRenderer {
    bgfx::ProgramHandle program{bgfx::kInvalidHandle};
    bgfx::UniformHandle uSampler{bgfx::kInvalidHandle};
    bgfx::VertexLayout layout{};
    bool initialized = false;

    static const char* shaderDir() noexcept {
    #ifdef RAVL2_SHADER_DIR
      return RAVL2_SHADER_DIR;
    #else
      return "";
    #endif
    }

    static std::string shaderPath(const char* base) {
      const bgfx::RendererType::Enum rt = bgfx::getRendererType();
      const char* ext = nullptr;
      switch (rt) {
        case bgfx::RendererType::OpenGL: ext = "glsl.bin"; break;
        case bgfx::RendererType::Vulkan: ext = "spv.bin"; break;
        default: ext = "glsl.bin"; break; // best effort
      }
      std::string p = std::string(shaderDir()) + "/" + base + "." + ext;
      return p;
    }

    static bgfx::ShaderHandle loadShaderFile(const std::string& path) {
      FILE* f = fopen(path.c_str(), "rb");
      if (!f) {
        SPDLOG_ERROR("NonImGuiBgfxRenderer: failed to open shader '{}'", path);
        return BGFX_INVALID_HANDLE;
      }
      fseek(f, 0, SEEK_END);
      long len = ftell(f);
      fseek(f, 0, SEEK_SET);
      if (len <= 0) { fclose(f); return BGFX_INVALID_HANDLE; }
      const bgfx::Memory* mem = bgfx::alloc(static_cast<uint32_t>(len + 1));
      if (fread(mem->data, 1, static_cast<size_t>(len), f) != static_cast<size_t>(len)) {
        fclose(f);
        return BGFX_INVALID_HANDLE;
      }
      fclose(f);
      mem->data[len] = '\0';
      return bgfx::createShader(mem);
    }

    bool init() {
      if (initialized) return true;
      // Vertex layout: vec2 position, vec2 uv
      layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
      uSampler = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
      if (!bgfx::isValid(uSampler)) {
        SPDLOG_ERROR("NonImGuiBgfxRenderer: failed to create sampler uniform");
        return false;
      }
      const std::string vs = shaderPath("vs_image");
      const std::string fs = shaderPath("fs_image");
      bgfx::ShaderHandle vsh = loadShaderFile(vs);
      bgfx::ShaderHandle fsh = loadShaderFile(fs);
      if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        if (bgfx::isValid(vsh)) bgfx::destroy(vsh);
        if (bgfx::isValid(fsh)) bgfx::destroy(fsh);
        SPDLOG_ERROR("NonImGuiBgfxRenderer: failed to load shaders ({} , {})", vs, fs);
        return false;
      }
      program = bgfx::createProgram(vsh, fsh, true /*destroy shaders*/);
      if (!bgfx::isValid(program)) {
        SPDLOG_ERROR("NonImGuiBgfxRenderer: failed to create program");
        return false;
      }
      initialized = true;
      SPDLOG_INFO("NonImGuiBgfxRenderer: program initialized (renderer={})", static_cast<int>(bgfx::getRendererType()));
      return true;
    }

    void shutdown() {
      if (bgfx::isValid(program)) { bgfx::destroy(program); program = BGFX_INVALID_HANDLE; }
      if (bgfx::isValid(uSampler)) { bgfx::destroy(uSampler); uSampler = BGFX_INVALID_HANDLE; }
      initialized = false;
    }

    void submitTexturedQuad(uint16_t fbw, uint16_t fbh,
                            uint16_t texHandleIdx,
                            int imgW, int imgH,
                            float sx, float sy, float tx, float ty) {
      if (!initialized && !init()) return;
      if (texHandleIdx == UINT16_MAX) return;
      bgfx::TextureHandle th{texHandleIdx};
      if (!bgfx::isValid(th)) return;

      // Compute destination rectangle in pixels using view2D params
      float dstW = static_cast<float>(imgW) * sx;
      float dstH = static_cast<float>(imgH) * sy;
      float dstX = tx; // pixels from left
      float dstY = ty; // pixels from top

      // Convert to NDC vertices (origin center, y up). SDL uses top-left origin.
      auto toNdcX = [&](float px){ return (px / float(fbw)) * 2.0f - 1.0f; };
      auto toNdcY = [&](float py){ return 1.0f - (py / float(fbh)) * 2.0f; };

      float x0 = toNdcX(dstX);
      float y0 = toNdcY(dstY);
      float x1 = toNdcX(dstX + dstW);
      float y1 = toNdcY(dstY + dstH);

      struct Vtx { float x,y,u,v; };
      Vtx* vtx = nullptr;
      const uint16_t numVerts = 4;
      const uint16_t numInds = 6;
      bgfx::TransientVertexBuffer tvb;
      bgfx::TransientIndexBuffer tib;
      if (bgfx::getAvailTransientVertexBuffer(numVerts, layout) < numVerts ||
          bgfx::getAvailTransientIndexBuffer(numInds) < numInds) {
        SPDLOG_WARN("NonImGuiBgfxRenderer: transient buffer alloc unavailable");
        return;
      }
      bgfx::allocTransientVertexBuffer(&tvb, numVerts, layout);
      bgfx::allocTransientIndexBuffer(&tib, numInds);
      vtx = reinterpret_cast<Vtx*>(tvb.data);
      // Triangle strip order (we'll use indices for two triangles)
      vtx[0] = { x0, y0, 0.0f, 0.0f }; // top-left
      vtx[1] = { x1, y0, 1.0f, 0.0f }; // top-right
      vtx[2] = { x1, y1, 1.0f, 1.0f }; // bottom-right
      vtx[3] = { x0, y1, 0.0f, 1.0f }; // bottom-left
      uint16_t* idx = reinterpret_cast<uint16_t*>(tib.data);
      idx[0]=0; idx[1]=1; idx[2]=2; idx[3]=0; idx[4]=2; idx[5]=3;

      uint64_t state = 0
        | BGFX_STATE_WRITE_RGB
        | BGFX_STATE_WRITE_A
        | BGFX_STATE_MSAA
        ;
      bgfx::setState(state);
      bgfx::setVertexBuffer(0, &tvb, 0, numVerts);
      bgfx::setIndexBuffer(&tib, 0, numInds);
      bgfx::setTexture(0, uSampler, th);
      bgfx::submit(0, program);
    }
  };

  NonImGuiBgfxRenderer g_nonImguiRenderer;
}
#endif

#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
// ImGui (bgfx path): UI API; bridge wraps bgfx backend. Keep SDL backends for fallback and include bgfx helper for button masks.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include "Ravl2/Display/bgfx_imgui/ImGUI/imgui.hh"
#include "Ravl2/Display/Commands/SetNormalization2D.hh"
#pragma GCC diagnostic pop
#elif defined(RAVL2_WITH_IMGUI)
// ImGui (SDL2 + SDL_Renderer2 backend)
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include "Ravl2/Display/Commands/SetNormalization2D.hh"
#endif

#ifdef __APPLE__
#include <objc/message.h>
#include <objc/runtime.h>
#endif

namespace Ravl2::DebugDisplay {

// Headless/test mode toggle
static std::atomic_bool g_headless{false};

void setHeadlessForTests(bool on) noexcept {
  g_headless.store(on, std::memory_order_release);
}

// Simple Clear command used by shim and controls parsing
namespace Commands {
  struct ClearChannelCommand : public IRenderCommand {
    std::string channel;
    explicit ClearChannelCommand(std::string ch) : channel(std::move(ch)) {}
    void apply(ChannelRegistry &channels) override {
      channels.clearChannel(channel);
      SPDLOG_INFO("DebugDisplay: cleared channel '{}'", channel);
    }
  };
}

namespace {
  std::once_flag g_startOnce;
  std::atomic_bool g_started{false};
  
#ifdef __APPLE__
  // On macOS, track whether we're running on main thread
  std::atomic_bool g_runningOnMainThread{false};
#endif

  // Constants for timings and UI layout
  constexpr int kHeartbeatMs = 33;
  constexpr int kMaxCommandsPerTick = 64;
  constexpr int kRendererInitWaitMs = 250;
  constexpr float kControlsPosX = 10.0f;
  constexpr float kControlsPosY = 10.0f;
  constexpr float kControlsInitialWidth = 360.0f;
  constexpr float kZoomMin = 0.05f;
  constexpr float kZoomMax = 32.0f;

  // Bounded queue of commands
  ThreadedQueue<std::shared_ptr<IRenderCommand>> g_queue{128};

  // Channel registry lives on the GUI thread
  ChannelRegistry g_channels;

  // Background thread (GUI thread)
  std::unique_ptr<std::jthread> g_guiThread;

  // SDL window and state (will be owned by SdlApp wrapper)
  SDL_Window* g_window = nullptr;
  SDL_Renderer* g_renderer = nullptr;
  std::atomic_bool g_invalidated{true};
  bool g_needSDLRenderer = true;

  // bgfx context (initialized against SDL window)
  BGFXContext g_bgfx;

  // Mouse interaction state
  bool g_dragging = false;
  int g_lastMouseX = 0;
  int g_lastMouseY = 0;
  std::string g_activeChannel; // channel under cursor or being dragged
  
  // Input controller for pan/zoom (Step 8)
  InputController2D g_input(g_invalidated, kZoomMin, kZoomMax);

  // Per-channel texture cache (SDL fallback when bgfx not initialized)
  struct TextureEntry {
    SDL_Texture* tex = nullptr;
    int w = 0;
    int h = 0;
  };
  std::unordered_map<std::string, TextureEntry> g_textures;
  std::unordered_map<std::string, SDL_FRect> g_lastRects; // last drawn image rect per channel (screen space)
  std::unordered_map<std::string, SDL_FPoint> g_imageOrigins; // per-channel image origin = content cursor screen pos
  std::unordered_map<std::string, SDL_FRect> g_contentRects; // per-channel window content rect (screen space)
  std::string g_hoveredChannel; // top-most hovered channel window name (if any)
  std::string g_hoveredImageChannel; // top-most hovered image item (channel) this frame

  // ImGui state
#if defined(RAVL2_WITH_IMGUI)
  bool g_imguiInitialized = false;
#endif
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
  ImguiBgfxBridge g_imguiBridge;
  uint8_t g_mouseButtons = 0;
  int32_t g_scroll = 0;
#endif

  // Forward declarations for mouse helpers used in event processing
  static inline void getMouseScreenPos(int& mx, int& my) noexcept;

  // Minimal SDL wrapper (Step 5): encapsulates init/shutdown/event processing without changing globals
  struct SdlApp {
    bool init() {
      // Note: SDL should already be initialized on the main thread before this is called
      if (SDL_WasInit(0) == 0) {
        SPDLOG_ERROR("DebugDisplay: SDL was not initialized before GUI thread started");
        return false;
      }
      // Create a resizable, high-DPI aware window
      // On macOS, add SDL_WINDOW_METAL to allow creation from non-main thread
      uint32_t windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN;
#ifdef __APPLE__
      windowFlags |= SDL_WINDOW_METAL;
#endif
      g_window = SDL_CreateWindow(
          "Ravl2 Debug Display",
          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
          1280, 720,
          windowFlags);
      if (!g_window) {
        SPDLOG_ERROR("DebugDisplay: SDL_CreateWindow failed: {}", SDL_GetError());
        return false;
      }

      // Determine initial window size
      int winW = 0, winH = 0;
      SDL_GetWindowSize(g_window, &winW, &winH);

      // Try bgfx first on non-Apple platforms, before creating any SDL_Renderer to avoid GL context conflicts
#if !defined(__APPLE__) || true
      BGFXContext::InitParams ip{};
      ip.backend = BGFXContext::Backend::Auto; // let bgfx choose the best available backend
      ip.width = winW > 0 ? winW : 1280;
      ip.height = winH > 0 ? winH : 720;
      ip.nativeWindow = g_window;
      if (!g_bgfx.init(ip)) {
        SPDLOG_WARN("DebugDisplay: bgfx init failed; falling back to SDL_Renderer path");
      }
#else
      SPDLOG_INFO("DebugDisplay: skipping bgfx init on macOS for now; using SDL renderer for images");
#endif

      // Decide if we need SDL renderer (fallback) when bgfx isn't usable for 2D blit
      g_needSDLRenderer = !g_bgfx.initialized();
#if defined(RAVL2_WITH_BGFX) && !defined(RAVL2_WITH_IMGUI)
      // If bgfx is initialized but non-ImGui renderer can't init (e.g., shaders missing), fall back to SDL
      if (g_bgfx.initialized()) {
        if (!g_nonImguiRenderer.init()) {
          SPDLOG_WARN("DebugDisplay: non-ImGui bgfx renderer not available; using SDL renderer fallback");
          g_needSDLRenderer = true;
        }
      }
#endif

      if (g_needSDLRenderer) {
#ifdef __APPLE__
        // Prefer Metal renderer on macOS for correct pixel format ordering
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
#endif
        // Prefer accelerated renderer with vsync; fallback to software if unavailable
        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!g_renderer) {
          // As a fallback, try accelerated with vsync (may still conflict with GL backends)
          SPDLOG_WARN("DebugDisplay: SDL_CreateRenderer (accelerated) failed ({}). Trying software.", SDL_GetError());
          g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
          if (!g_renderer) {
            SPDLOG_ERROR("DebugDisplay: SDL_CreateRenderer failed: {}", SDL_GetError());
            return false;
          }
        }

        // Some platforms (Wayland/HiDPI) report a 0x0 drawable until the first expose/resize.
        // Wait briefly for a non-zero renderer output size so the first frame is visible without moving the window.
        SDL_PumpEvents();
        int outW = 0, outH = 0;
        const uint32_t startTicks = SDL_GetTicks();
        while (true) {
          SDL_GetRendererOutputSize(g_renderer, &outW, &outH);
          if (outW > 0 && outH > 0) break;
          if (SDL_GetTicks() - startTicks > kRendererInitWaitMs) { // give up after configured ms
            break;
          }
          // Wait for any window config event briefly
          SDL_WaitEventTimeout(nullptr, 5);
        }
        if (outW <= 0 || outH <= 0) {
          SPDLOG_WARN("DebugDisplay: renderer output size is {}x{} at startup; first frame may be delayed.", outW, outH);
        } else {
          SPDLOG_INFO("DebugDisplay: SDL window+renderer created (drawable {}x{})", outW, outH);
        }
      } else {
        SPDLOG_INFO("DebugDisplay: bgfx initialized; SDL_Renderer not created to avoid conflicts");
      }

      // Initialize Dear ImGui
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
      if (!g_imguiInitialized) {
        if (g_bgfx.initialized()) {
          if (auto res = g_imguiBridge.initEx(18.0f); res.has_value()) {
            // Only move windows from their title bars to avoid accidental drags while panning images.
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigWindowsMoveFromTitleBarOnly = true;
            g_imguiInitialized = true;
            SPDLOG_INFO("DebugDisplay: Dear ImGui initialized (bgfx backend via ImguiBgfxBridge)");
          } else {
            SPDLOG_WARN("DebugDisplay: ImguiBgfxBridge init failed while bgfx is initialized: {}", res.error());
          }
        } else {
          IMGUI_CHECKVERSION();
          ImGui::CreateContext();
          ImGuiIO& io = ImGui::GetIO(); (void)io;
          io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
          io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
          ImGui::StyleColorsDark();
          if (!ImGui_ImplSDL2_InitForSDLRenderer(g_window, g_renderer)) {
            SPDLOG_WARN("DebugDisplay: ImGui_ImplSDL2_InitForSDLRenderer failed");
          }
          if (!ImGui_ImplSDLRenderer2_Init(g_renderer)) {
            SPDLOG_WARN("DebugDisplay: ImGui_ImplSDLRenderer2_Init failed");
          } else {
            g_imguiInitialized = true;
            SPDLOG_INFO("DebugDisplay: Dear ImGui initialized (SDL_Renderer backend, bgfx init failed)");
          }
        }
      }
#elif defined(RAVL2_WITH_IMGUI)
      if (!g_imguiInitialized) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::StyleColorsDark();
        if (!ImGui_ImplSDL2_InitForSDLRenderer(g_window, g_renderer)) {
          SPDLOG_WARN("DebugDisplay: ImGui_ImplSDL2_InitForSDLRenderer failed");
        }
        if (!ImGui_ImplSDLRenderer2_Init(g_renderer)) {
          SPDLOG_WARN("DebugDisplay: ImGui_ImplSDLRenderer2_Init failed");
        } else {
          g_imguiInitialized = true;
          SPDLOG_INFO("DebugDisplay: Dear ImGui initialized (SDL_Renderer backend)");
        }
      }
#endif

      // Invalidate to force an immediate first render once the loop starts
      g_invalidated.store(true, std::memory_order_release);
      return true;
    }

    void shutdown() {
      // Shutdown ImGui if initialized
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
      if (g_imguiInitialized) {
        g_imguiBridge.shutdown();
        g_imguiInitialized = false;
      }
#elif defined(RAVL2_WITH_IMGUI)
      if (g_imguiInitialized) {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        g_imguiInitialized = false;
      }
#endif

      // Clean up SDL textures (used for fallback rendering)
      for (auto &kv : g_textures) {
        if (kv.second.tex) SDL_DestroyTexture(kv.second.tex);
      }
      g_textures.clear();

      if (g_renderer) { SDL_DestroyRenderer(g_renderer); g_renderer = nullptr; }
      if (g_window) { SDL_DestroyWindow(g_window); g_window = nullptr; }
      if (SDL_WasInit(0)) { SDL_Quit(); }
    }

    void processEvents(std::stop_token st) {
      (void)st; // unused for now; reserved for future stop-aware processing
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
#if defined(RAVL2_WITH_IMGUI) && !defined(RAVL2_WITH_BGFX)
        // Forward events to ImGui (SDL backend)
        ImGui_ImplSDL2_ProcessEvent(&e);
#endif
        if (e.type == SDL_QUIT) {
          // Request stop on quit
          if (g_guiThread) { g_guiThread->request_stop(); }
          return;
        }
        if (e.type == SDL_WINDOWEVENT) {
          if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
              e.window.event == SDL_WINDOWEVENT_RESIZED ||
              e.window.event == SDL_WINDOWEVENT_SHOWN ||
              e.window.event == SDL_WINDOWEVENT_EXPOSED) {
            if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || e.window.event == SDL_WINDOWEVENT_RESIZED) {
              const int nw = static_cast<int>(e.window.data1);
              const int nh = static_cast<int>(e.window.data2);
              if (nw > 0 && nh > 0) {
                g_bgfx.resize(nw, nh);
              }
            }
            g_invalidated.store(true, std::memory_order_release);
          } else if (e.window.event == SDL_WINDOWEVENT_LEAVE ||
                     e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            // Safety: cancel any ongoing drag if the mouse leaves the window or focus is lost.
            g_dragging = false;
            g_activeChannel.clear();
            g_input.onMouseButtonUp(SDL_BUTTON_LEFT);
          }
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
          if (e.button.button == SDL_BUTTON_LEFT) {
            g_dragging = true;
            int mx=0,my=0; getMouseScreenPos(mx,my);
            g_lastMouseX = mx;
            g_lastMouseY = my;
            g_activeChannel.clear();
            // Start drag only on the top-most hovered image item (if any)
            if (!g_hoveredImageChannel.empty()) {
              g_input.onMouseButtonDown(mx, my, g_hoveredImageChannel, g_lastRects, g_contentRects);
              g_activeChannel = g_input.activeChannel();
            }
          }
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
          if (e.button.button == SDL_BUTTON_LEFT) g_mouseButtons |= IMGUI_MBUT_LEFT;
          if (e.button.button == SDL_BUTTON_RIGHT) g_mouseButtons |= IMGUI_MBUT_RIGHT;
          if (e.button.button == SDL_BUTTON_MIDDLE) g_mouseButtons |= IMGUI_MBUT_MIDDLE;
#endif
        } else if (e.type == SDL_MOUSEBUTTONUP) {
          if (e.button.button == SDL_BUTTON_LEFT) {
            g_dragging = false;
          }
          // Always notify our input controller on button release to stop any panning/dragging,
          // regardless of ImGui capture state.
          g_input.onMouseButtonUp(e.button.button);
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
          if (e.button.button == SDL_BUTTON_LEFT) g_mouseButtons &= static_cast<uint8_t>(~IMGUI_MBUT_LEFT);
          if (e.button.button == SDL_BUTTON_RIGHT) g_mouseButtons &= static_cast<uint8_t>(~IMGUI_MBUT_RIGHT);
          if (e.button.button == SDL_BUTTON_MIDDLE) g_mouseButtons &= static_cast<uint8_t>(~IMGUI_MBUT_MIDDLE);
#endif
        } else if (e.type == SDL_MOUSEMOTION) {
          {
            int mx=0,my=0; getMouseScreenPos(mx,my);
            // Only move the image that started the drag (activeChannel)
            if (!g_activeChannel.empty() && g_activeChannel == g_hoveredImageChannel) {
              g_input.onMouseMotion(mx, my, g_channels);
            }
          }
        } else if (e.type == SDL_MOUSEWHEEL) {
          // Zoom in/out around cursor for channel under mouse
          int mx=0,my=0; getMouseScreenPos(mx,my);
          // Zoom only the top-most hovered image item (if any)
          if (!g_hoveredImageChannel.empty()) {
            g_input.onMouseWheel(e.wheel.y, mx, my, g_hoveredImageChannel, g_lastRects, g_contentRects, g_channels);
          }
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
          // Forward scroll to ImGui bgfx backend (accumulate this frame)
          g_scroll += e.wheel.y;
#endif
        }
      }
    }
  };

  // Mouse helpers: fetch screen-space mouse position and test against channel rects
  static inline void getMouseScreenPos(int& mx, int& my) noexcept {
  #if defined(RAVL2_WITH_IMGUI)
    if (g_imguiInitialized) {
      ImVec2 mp = ImGui::GetMousePos();
      mx = static_cast<int>(mp.x);
      my = static_cast<int>(mp.y);
      return;
    }
  #endif
    SDL_GetMouseState(&mx, &my);
  }

  static SdlApp g_sdlApp; // single instance for GUI thread

  static void ensureTextureFor(const std::string &channel, int w, int h)
  {
    auto &entry = g_textures[channel];
    if (entry.tex && (entry.w != w || entry.h != h)) {
      SDL_DestroyTexture(entry.tex);
      entry.tex = nullptr; entry.w = entry.h = 0;
    }
    if (!entry.tex) {
      // Use a widely supported 32-bit texture; prefer BGRA on macOS/Metal to avoid swizzle issues.
#ifdef __APPLE__
      Uint32 pf = SDL_PIXELFORMAT_ABGR8888; // corresponds to BGRA byte-order on little-endian
#else
      Uint32 pf = SDL_PIXELFORMAT_RGBA8888;
#endif
      entry.tex = SDL_CreateTexture(g_renderer, pf, SDL_TEXTUREACCESS_STREAMING, w, h);
      if (!entry.tex) {
        SPDLOG_ERROR("DebugDisplay: SDL_CreateTexture failed: {}", SDL_GetError());
        return;
      }
      entry.w = w; entry.h = h;
    }
  }

  static void uploadGrayscaleToTexture(SDL_Texture* tex, const uint8_t* gray, int w, int h)
  {
    void* pixels = nullptr; int pitch = 0;
    if (SDL_LockTexture(tex, nullptr, &pixels, &pitch) != 0) {
      SPDLOG_ERROR("DebugDisplay: SDL_LockTexture failed: {}", SDL_GetError());
      return;
    }
    // Expand to RGBA8888
    auto *dst = static_cast<uint8_t*>(pixels);
    for (int y=0; y<h; ++y) {
      uint8_t* row = dst + y * pitch;
      const uint8_t* srcRow = gray + y * w;
      for (int x=0; x<w; ++x) {
        uint8_t v = srcRow[x];
        size_t off = static_cast<size_t>(x) * 4;
        row[off + 0] = v; // R
        row[off + 1] = v; // G
        row[off + 2] = v; // B
        row[off + 3] = 255; // A
      }
    }
    SDL_UnlockTexture(tex);
  }

  static void uploadFloatToTexture(SDL_Texture* tex, const float* f32, int w, int h, float mn, float mx)
  {
    if (mx <= mn) { mx = mn + 1.0f; }
    std::vector<uint8_t> tmp(static_cast<size_t>(w)*static_cast<size_t>(h));
    for (int i=0, n=w*h; i<n; ++i) {
      float v = (f32[i] - mn) / (mx - mn);
      if (v < 0.0f) v = 0.0f;
      if (v > 1.0f) v = 1.0f;
      tmp[static_cast<size_t>(i)] = static_cast<uint8_t>(v * 255.0f + 0.5f);
    }
    uploadGrayscaleToTexture(tex, tmp.data(), w, h);
  }

  void renderAll()
  {
    // SDL fallback rendering (used when bgfx is not initialized)
    if (!g_renderer) return;
    SDL_SetRenderDrawColor(g_renderer, 16, 16, 24, 255);
    SDL_RenderClear(g_renderer);

    // Clear last rects before drawing
    g_lastRects.clear();

    // Enumerate channels and render their base images with view transform
    g_channels.forEachChannel([](ChannelState &ch){
      if (!ch.sceneContent) return;
      auto *node = dynamic_cast<Image2DNodeBase*>(ch.sceneContent.get());
      if (!node) return;
      const int w = node->width, h = node->height;
      if (w <= 0 || h <= 0) return;

      ensureTextureFor(ch.name, w, h);
      auto it = g_textures.find(ch.name);
      if (it == g_textures.end() || !it->second.tex) return;

      // Try uint8 node
      if (auto* u8node = dynamic_cast<Image2DNode<uint8_t>*>(node)) {
        const auto& data = u8node->getData();
        if (!data.empty()) {
          uploadGrayscaleToTexture(it->second.tex, data.data(), w, h);
        }
      }
      // Try float node
      else if (auto* f32node = dynamic_cast<Image2DNode<float>*>(node)) {
        const auto& data = f32node->getData();
        if (!data.empty()) {
          // Choose normalization based on channel settings
          float mn = f32node->getCachedMin(), mx = f32node->getCachedMax();
          switch (ch.norm.policy) {
            case NormalizationPolicy::Auto:
              // already set via cachedMin/Max
              break;
            case NormalizationPolicy::Fixed:
              mn = ch.norm.minVal; mx = ch.norm.maxVal; break;
            case NormalizationPolicy::Percentile: {
              auto mm = percentiles(data.data(), w, h, w*int(sizeof(float)), ch.norm.lowPct, ch.norm.highPct);
              mn = mm.first; mx = mm.second; break; }
          }
          uploadFloatToTexture(it->second.tex, data.data(), w, h, mn, mx);
        }
      } else {
        return;
      }

      // Compute destination rect from view2D (scale+translate)
      const auto &st = ch.view2D;
      const float sx = st.scaleVector()[0];
      const float sy = st.scaleVector()[1];
      const float tx = st.translation()[0];
      const float ty = st.translation()[1];
      SDL_FRect dst { tx, ty, static_cast<float>(w) * sx, static_cast<float>(h) * sy };
      g_lastRects[ch.name] = dst;

      SDL_RenderCopyF(g_renderer, it->second.tex, nullptr, &dst);
    });

    // Note: SDL_RenderPresent is called after ImGui rendering in guiThreadMain.
  }

  [[maybe_unused]] void renderAllBgfxNonImGui(uint16_t fbw, uint16_t fbh)
  {
#if defined(RAVL2_WITH_BGFX)
    g_lastRects.clear();
    RenderContext rc{}; rc.framebufferWidth = fbw; rc.framebufferHeight = fbh;
    g_channels.forEachChannel([&](ChannelState &ch){
      if (!ch.sceneContent) return;
      auto *node = dynamic_cast<Image2DNodeBase*>(ch.sceneContent.get());
      if (!node) return;
      node->prepare(rc);
      if (node->width > 0 && node->height > 0 && node->textureHandleIdx != UINT16_MAX) {
        const float sx = ch.view2D.scaleVector()[0];
        const float sy = ch.view2D.scaleVector()[1];
        const float tx = ch.view2D.translation()[0];
        const float ty = ch.view2D.translation()[1];
        g_nonImguiRenderer.submitTexturedQuad(fbw, fbh, node->textureHandleIdx, node->width, node->height, sx, sy, tx, ty);
        SDL_FRect r{ tx, ty, static_cast<float>(node->width) * sx, static_cast<float>(node->height) * sy };
        g_lastRects[ch.name] = r;
      }
    });
#endif
  }



  // --- Loop helpers extracted in Phase 4.9 Step 4 ---
  static inline void drainCommandsOnce(int maxPerTick)
  {
    int drained = 0;
    std::shared_ptr<IRenderCommand> cmd;
    while (drained < maxPerTick && g_queue.tryPop(cmd)) {
      if (cmd) { cmd->apply(g_channels); g_invalidated.store(true, std::memory_order_release); }
      ++drained;
    }
  }

  static void runImGuiFrame()
  {
  #if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
    if (g_imguiInitialized && g_bgfx.initialized())
    {
      int mx=0, my=0;
      SDL_GetMouseState(&mx, &my);
      uint16_t fbw = 0, fbh = 0;
      int winW=0, winH=0;
      SDL_GetWindowSize(g_window, &winW, &winH);
      fbw = static_cast<uint16_t>(winW);
      fbh = static_cast<uint16_t>(winH);
      SPDLOG_DEBUG("ImGui frame begin: fb={}x{}, mouse=({},{}), buttons=0x{:x}, scroll={}", fbw, fbh, mx, my, static_cast<unsigned>(g_mouseButtons), static_cast<int>(g_scroll));
      g_imguiBridge.beginFrame(mx, my, g_mouseButtons, g_scroll, fbw, fbh);
      g_scroll = 0; // consume scroll

      Ui::buildDockspace();

      // Plots panel (Phase 4.9 Step 12: placeholder hooks for future ImPlot integration)
      Ui::Plots::buildPlotsPanel();

      // Controls window (now dockable; only position on first use)
      ImGui::SetNextWindowPos(ImVec2(kControlsPosX, kControlsPosY), ImGuiCond_Once);
      ImGui::SetNextWindowSize(ImVec2(kControlsInitialWidth, 0), ImGuiCond_FirstUseEver);
      ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
      SPDLOG_DEBUG("ImGui Controls window built");
      Ui::buildControlsPanel(static_cast<float>(fbw), static_cast<float>(fbh),
                             g_channels,
                             [&](std::shared_ptr<IRenderCommand> cmd){ g_queue.push(std::move(cmd)); },
                             g_invalidated,
                             kZoomMin, kZoomMax);
      ImGui::End();

      Ui::ChannelWindows::build(fbw, fbh, g_channels, g_lastRects, g_imageOrigins, g_contentRects, g_invalidated, g_hoveredChannel, g_hoveredImageChannel);

      // Bottom status bar with live inspector info
      Ui::StatusBar::build(g_lastRects, g_imageOrigins, g_channels);

      g_imguiBridge.endFrame();
      SPDLOG_DEBUG("ImGui frame submitted");
    }
  #elif defined(RAVL2_WITH_IMGUI)
    if (g_imguiInitialized) {
      ImGui_ImplSDL2_NewFrame();
      ImGui_ImplSDLRenderer2_NewFrame();
      ImGui::NewFrame();

      Ui::buildDockspace();

      // Plots panel (Phase 4.9 Step 12: placeholder hooks for future ImPlot integration)
      Ui::Plots::buildPlotsPanel();

      ImGui::SetNextWindowPos(ImVec2(kControlsPosX, kControlsPosY), ImGuiCond_Once);
      ImGui::SetNextWindowSize(ImVec2(kControlsInitialWidth, 0), ImGuiCond_FirstUseEver);
      if (ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        int winW=0, winH=0; SDL_GetWindowSize(g_window, &winW, &winH);
        Ui::buildControlsPanel(static_cast<float>(winW), static_cast<float>(winH),
                               g_channels,
                               [&](std::shared_ptr<IRenderCommand> cmd){ g_queue.push(std::move(cmd)); },
                               g_invalidated,
                               kZoomMin, kZoomMax);
      }
      ImGui::End();

      // Bottom status bar with live inspector info (SDL path)
      Ui::StatusBar::build(g_lastRects, g_imageOrigins, g_channels);

      ImGui::Render();
      ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), g_renderer);
    }
  #endif
  }

  static inline void renderFallbackIfNeeded()
  {
    if (!g_bgfx.initialized()) {
      g_invalidated.exchange(false, std::memory_order_acq_rel);
      renderAll();
    }
  }

  static inline void renderNonImguiBgfxIfEnabled()
  {
  #if defined(RAVL2_WITH_BGFX) && !defined(RAVL2_WITH_IMGUI)
    if (g_bgfx.initialized()) {
      int winW=0, winH=0; SDL_GetWindowSize(g_window, &winW, &winH);
      uint16_t fbw = static_cast<uint16_t>(winW);
      uint16_t fbh = static_cast<uint16_t>(winH);
      renderAllBgfxNonImGui(fbw, fbh);
    }
  #endif
  }

  void guiThreadMain(std::stop_token st)
  {
    SPDLOG_INFO("DebugDisplay: GUI thread started (SDL window+renderer)");
    if (!g_sdlApp.init()) {
      SPDLOG_ERROR("DebugDisplay: failed to initialize SDL window/renderer; GUI thread exiting");
      return;
    }

    while(!st.stop_requested()) {
      // Wait for events with timeout to keep CPU low when idle
      SDL_WaitEventTimeout(nullptr, kHeartbeatMs); // ~30 FPS heartbeat
      if (st.stop_requested()) {
        SPDLOG_INFO("DebugDisplay: GUI loop detected stop request, exiting");
#if defined(RAVL2_WITH_BGFX)
        if (g_bgfx.initialized()) {
          // Disable bgfx debug markers and flush a final frame to avoid GL debug group calls after teardown
          bgfx::setDebug(BGFX_DEBUG_NONE);
          bgfx::touch(0);
          g_bgfx.frame();
        }
#endif
        break; // Check immediately after wait
      }
      g_sdlApp.processEvents(st);

      // Drain commands and run one UI/render tick
      drainCommandsOnce(kMaxCommandsPerTick);
      runImGuiFrame();
      renderFallbackIfNeeded();
      renderNonImguiBgfxIfEnabled();

      // Render UI and present
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
      if (g_imguiInitialized && g_bgfx.initialized()) {
        // ImGui bgfx backend submits during imguiEndFrame via bgfx calls; nothing to do here
      }
#elif defined(RAVL2_WITH_IMGUI)
      if (g_imguiInitialized) {
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), g_renderer);
      }
#endif
      // Use SDL_RenderPresent when bgfx is not initialized
      if (!g_bgfx.initialized()) {
        SDL_RenderPresent(g_renderer);
      }

#if defined(RAVL2_WITH_BGFX)
      // Show bgfx debug text only when ImGui is not initialized to avoid confusing draw order during diagnosis.
  #if defined(RAVL2_WITH_IMGUI)
      if (g_bgfx.initialized() && !g_imguiInitialized) {
  #else
      if (g_bgfx.initialized()) {
  #endif
        bgfx::dbgTextClear();
        bgfx::dbgTextPrintf(0, 0, 0x0f, "Ravl2 DebugDisplay — backend=%s", BGFXContext::backendName(g_bgfx.backend()));
        bgfx::touch(0);
      }
#endif

      // Submit bgfx frame boundary (only if initialized)
      if (g_bgfx.initialized()) {
        g_bgfx.frame();
      }
    }

    // Destroy ImGui (bgfx backend) before shutting down bgfx to avoid use-after-shutdown
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
    if (g_imguiInitialized && g_bgfx.initialized()) {
      g_imguiBridge.shutdown();
      g_imguiInitialized = false;
    }
#endif
    // Shutdown bgfx before SDL teardown
    g_bgfx.shutdown();

    g_sdlApp.shutdown();
    SPDLOG_INFO("DebugDisplay: GUI thread exiting");
  }
}

void ensureStarted(const InitOptions &opts) {
  (void)opts;
  std::call_once(g_startOnce, []() {
    // Note: SDL should be initialized on the main thread before this is called (via initDisplay())
#ifdef __APPLE__
    // On macOS, when using RAVL2_MAIN, the GUI thread is the main thread
    // Skip creating a background thread; GUI will run on main via runMainLoop
    if (g_runningOnMainThread) {
      SPDLOG_INFO("DebugDisplay: will run on main thread (macOS)");
      g_started.store(true, std::memory_order_release);
      return;
    }
#endif
    // Headless mode: do not start GUI thread or create a window
    if (g_headless.load(std::memory_order_acquire)) {
      SPDLOG_INFO("DebugDisplay: headless mode enabled — no GUI thread/window");
      g_started.store(true, std::memory_order_release);
      return;
    }
    // Start background GUI thread (SDL window + simple renderer)
    g_guiThread = std::make_unique<std::jthread>(guiThreadMain);
    SPDLOG_INFO("DebugDisplay: starting — background GUI thread created");
    g_started.store(true, std::memory_order_release);
    // Ensure clean shutdown
    std::atexit([](){
      if (g_guiThread) {
        g_guiThread->request_stop();
        // Let the thread run down; destructor will join
        g_guiThread.reset();
      }
      SPDLOG_INFO("DebugDisplay: stopped");
    });
  });
}

std::expected<void, std::string> enqueue(std::shared_ptr<IRenderCommand> command)
{
  if (!g_started.load(std::memory_order_acquire)) { ensureStarted({}); }
  if (!command) { return std::unexpected(std::string{"DebugDisplay enqueue: null command"}); }

  if(!g_queue.tryPush(std::move(command))) {
    SPDLOG_WARN("DebugDisplay: queue overflow; dropping command");
  } else {
    g_invalidated.store(true, std::memory_order_release);
  }
  return {};
}

// Deprecated shim: type-erased payload path — currently only honors :Clear
std::expected<void, std::string> enqueue(std::string_view channel,
                                         std::type_index type,
                                         std::shared_ptr<const void> payload,
                                         uint32_t flags,
                                         std::string_view controls)
{
  (void)type; (void)payload; (void)flags;
  // Convert :Clear control to a command; otherwise no-op and success
  std::shared_ptr<IRenderCommand> cmd;
  const bool hasClearCtrl = (!controls.empty() && controls.find(":Clear") != std::string_view::npos);
  if (hasClearCtrl) {
    cmd = std::make_shared<Commands::ClearChannelCommand>(std::string{channel});
    return enqueue(cmd);
  }
  // No recognized control; accept as no-op for backward compatibility
  return {};
}

int runMainLoop(int (*appMain)(int, char**), int argc, char** argv)
{
#ifdef __APPLE__
  g_runningOnMainThread.store(true, std::memory_order_release);
  
  // Initialize SDL on the main thread (must happen here on macOS)
  // This is done outside RenderCommandSink::initDisplay since we need it before app thread
  if (SDL_WasInit(0) == 0) {
    SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "0");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
      SPDLOG_ERROR("DebugDisplay: SDL_Init failed on main thread: {}", SDL_GetError());
      return 1;
    }
    SPDLOG_INFO("DebugDisplay: SDL initialized on main thread (via runMainLoop)");
  }
  
  // Start the application main in a background thread
  int exitCode = 0;
  std::stop_source stopSource;
  std::stop_token stopToken = stopSource.get_token();
  
  std::thread appThread([&]() {
    exitCode = appMain(argc, argv);
    // Signal the GUI thread to stop when app completes
    SPDLOG_INFO("DebugDisplay: app thread finished, requesting GUI stop");
    stopSource.request_stop();
    SPDLOG_INFO("DebugDisplay: stop requested, stop_requested={}", stopToken.stop_requested());
  });
  
  // Wait for ensureStarted to be called by the app thread
  while (!g_started.load(std::memory_order_acquire)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  
  // Run the GUI loop on main thread until app signals stop
  guiThreadMain(stopToken);
  
  // Wait for app thread to complete
  if (appThread.joinable()) {
    appThread.join();
  }
  
  return exitCode;
#else
  // On non-macOS platforms, just run appMain directly
  return appMain(argc, argv);
#endif
}

} // namespace Ravl2::DebugDisplay
