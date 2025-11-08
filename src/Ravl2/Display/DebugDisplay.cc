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
#if defined(RAVL2_WITH_BGFX)
#include <bgfx/bgfx.h>
#endif

#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
// ImGui (Step B): bgfx backend (from bgfx examples) with manual input forwarding
#include "bgfx_imgui/imgui.h"
#include <imgui.h>
#endif

namespace Ravl2::DebugDisplay {

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

  // Bounded queue of commands
  ThreadedQueue<std::shared_ptr<IRenderCommand>> g_queue{128};

  // Channel registry lives on the GUI thread
  ChannelRegistry g_channels;

  // Background thread (GUI thread)
  std::unique_ptr<std::jthread> g_guiThread;

  // SDL window and state
  SDL_Window* g_window = nullptr;
  SDL_Renderer* g_renderer = nullptr;
  std::atomic_bool g_invalidated{true};

  // bgfx context (initialized against SDL window)
  BGFXContext g_bgfx;

  // Mouse interaction state
  bool g_dragging = false;
  int g_lastMouseX = 0;
  int g_lastMouseY = 0;
  std::string g_activeChannel; // channel under cursor or being dragged

  // Per-channel texture cache (SDL fallback only)
#if !defined(RAVL2_WITH_BGFX)
  struct TextureEntry {
    SDL_Texture* tex = nullptr;
    int w = 0;
    int h = 0;
  };
  std::unordered_map<std::string, TextureEntry> g_textures;
#endif
  std::unordered_map<std::string, SDL_FRect> g_lastRects; // last drawn rect per channel for hit-testing

  // ImGui state
#if defined(RAVL2_WITH_IMGUI)
  bool g_imguiInitialized = false;
#endif
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
  uint8_t g_mouseButtons = 0;
  int32_t g_scroll = 0;
#endif

  bool initSDLAndWindow()
  {
    if (SDL_WasInit(0) == 0) {
      if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SPDLOG_ERROR("DebugDisplay: SDL_Init failed: {}", SDL_GetError());
        return false;
        }
    }
    // Create a resizable, high-DPI aware window
    g_window = SDL_CreateWindow(
        "Ravl2 Debug Display",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
    if (!g_window) {
      SPDLOG_ERROR("DebugDisplay: SDL_CreateWindow failed: {}", SDL_GetError());
      return false;
    }

    // Create SDL renderer (accelerated if available)
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer) {
      SPDLOG_WARN("DebugDisplay: SDL_CreateRenderer failed ({}). Falling back to software.", SDL_GetError());
      g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
      if (!g_renderer) {
        SPDLOG_ERROR("DebugDisplay: SDL_CreateRenderer (software) failed: {}", SDL_GetError());
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
      if (SDL_GetTicks() - startTicks > 250) { // give up after 250ms
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

    // Initialize bgfx context (Step A: init only; SDL used for blit until Step B)
    BGFXContext::InitParams ip{};
    ip.backend = BGFXContext::Backend::Vulkan; // default; bgfx may auto-fallback inside
    ip.width = outW > 0 ? outW : 1280;
    ip.height = outH > 0 ? outH : 720;
    ip.nativeWindow = g_window;
    if (!g_bgfx.init(ip)) {
      SPDLOG_WARN("DebugDisplay: bgfx init failed; continuing with SDL renderer MVP only");
    }

    // Initialize Dear ImGui
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
    if (!g_imguiInitialized) {
      if (g_bgfx.initialized()) {
        imguiCreate(18.0f, nullptr);
        g_imguiInitialized = true;
        SPDLOG_INFO("DebugDisplay: Dear ImGui initialized (bgfx backend)");
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
        if (!ImGui_ImplSDLRenderer_Init(g_renderer)) {
          SPDLOG_WARN("DebugDisplay: ImGui_ImplSDLRenderer_Init failed");
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
      if (!ImGui_ImplSDLRenderer_Init(g_renderer)) {
        SPDLOG_WARN("DebugDisplay: ImGui_ImplSDLRenderer_Init failed");
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

  void shutdownSDL()
  {
    // Shutdown ImGui if initialized
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
    if (g_imguiInitialized) {
      imguiDestroy();
      g_imguiInitialized = false;
    }
#elif defined(RAVL2_WITH_IMGUI)
    if (g_imguiInitialized) {
      ImGui_ImplSDLRenderer_Shutdown();
      ImGui_ImplSDL2_Shutdown();
      ImGui::DestroyContext();
      g_imguiInitialized = false;
    }
#endif

#if !defined(RAVL2_WITH_BGFX)
    for (auto &kv : g_textures) {
      if (kv.second.tex) SDL_DestroyTexture(kv.second.tex);
    }
    g_textures.clear();
#endif

    if (g_renderer) { SDL_DestroyRenderer(g_renderer); g_renderer = nullptr; }
    if (g_window) { SDL_DestroyWindow(g_window); g_window = nullptr; }
    if (SDL_WasInit(0)) { SDL_Quit(); }
  }

  void processEvents(std::stop_token st)
  {
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
        }
      } else if (e.type == SDL_MOUSEBUTTONDOWN) {
        if (e.button.button == SDL_BUTTON_LEFT) {
          g_dragging = true;
          g_lastMouseX = e.button.x;
          g_lastMouseY = e.button.y;
          // Pick active channel under cursor
          g_activeChannel.clear();
          for (const auto &kv : g_lastRects) {
            const auto &r = kv.second;
            const float bx = static_cast<float>(e.button.x);
            const float by = static_cast<float>(e.button.y);
            if (bx >= r.x && bx < r.x + r.w &&
                by >= r.y && by < r.y + r.h) {
              g_activeChannel = kv.first;
              break;
            }
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
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
        if (e.button.button == SDL_BUTTON_LEFT) g_mouseButtons &= ~IMGUI_MBUT_LEFT;
        if (e.button.button == SDL_BUTTON_RIGHT) g_mouseButtons &= ~IMGUI_MBUT_RIGHT;
        if (e.button.button == SDL_BUTTON_MIDDLE) g_mouseButtons &= ~IMGUI_MBUT_MIDDLE;
#endif
      } else if (e.type == SDL_MOUSEMOTION) {
        if (g_dragging && !g_activeChannel.empty()) {
          int mx = e.motion.x;
          int my = e.motion.y;
          int dx = mx - g_lastMouseX;
          int dy = my - g_lastMouseY;
          g_lastMouseX = mx; g_lastMouseY = my;
          // Apply to channel translation
          auto &ch = g_channels.getOrCreateChannel(g_activeChannel);
          auto &t = ch.view2D.translation();
          t[0] += static_cast<float>(dx);
          t[1] += static_cast<float>(dy);
          g_invalidated.store(true, std::memory_order_release);
        }
      } else if (e.type == SDL_MOUSEWHEEL) {
        // Zoom in/out around cursor for active channel under cursor
        int mx, my; SDL_GetMouseState(&mx, &my);
        std::string under;
        for (const auto &kv : g_lastRects) {
          const auto &r = kv.second;
          const float fx = static_cast<float>(mx);
          const float fy = static_cast<float>(my);
          if (fx >= r.x && fx < r.x + r.w && fy >= r.y && fy < r.y + r.h) { under = kv.first; break; }
        }
        if (!under.empty()) {
          auto &ch = g_channels.getOrCreateChannel(under);
          auto &view = ch.view2D;
          float &sx = view.scaleVector()[0];
          float &sy = view.scaleVector()[1];
          float &tx = view.translation()[0];
          float &ty = view.translation()[1];
          float factor = (e.wheel.y > 0) ? 1.1f : 1.0f / 1.1f;
          float newSx = std::clamp(sx * factor, 0.05f, 32.0f);
          float newSy = std::clamp(sy * factor, 0.05f, 32.0f);
          // Compute image coords under cursor
          float ix = (static_cast<float>(mx) - tx) / (sx != 0.0f ? sx : 1.0f);
          float iy = (static_cast<float>(my) - ty) / (sy != 0.0f ? sy : 1.0f);
          // Update translation so the point under cursor remains fixed
          tx = static_cast<float>(mx) - ix * newSx;
          ty = static_cast<float>(my) - iy * newSy;
          sx = newSx; sy = newSy;
          g_invalidated.store(true, std::memory_order_release);
        }
      }
    }
  }

  static void ensureTextureFor(const std::string &channel, int w, int h)
  {
    auto &entry = g_textures[channel];
    if (entry.tex && (entry.w != w || entry.h != h)) {
      SDL_DestroyTexture(entry.tex);
      entry.tex = nullptr; entry.w = entry.h = 0;
    }
    if (!entry.tex) {
      // Use a widely supported 32-bit RGBA texture; we'll expand grayscale on upload.
      entry.tex = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, w, h);
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
#if !defined(RAVL2_WITH_BGFX)
    if (!g_renderer) return;
    SDL_SetRenderDrawColor(g_renderer, 16, 16, 24, 255);
    SDL_RenderClear(g_renderer);

    // Clear last rects before drawing
    g_lastRects.clear();

    // Enumerate channels and render their base images with view transform
    g_channels.forEachChannel([](ChannelState &ch){
      if (!ch.baseImage2D) return;
      auto *node = static_cast<Image2DNode*>(ch.baseImage2D.get());
      const int w = node->width, h = node->height;
      if (w <= 0 || h <= 0) return;

      ensureTextureFor(ch.name, w, h);
      auto it = g_textures.find(ch.name);
      if (it == g_textures.end() || !it->second.tex) return;

      if (node->format == Image2DFormat::U8 && !node->dataU8.empty()) {
        uploadGrayscaleToTexture(it->second.tex, node->dataU8.data(), w, h);
      } else if (node->format == Image2DFormat::F32 && !node->dataF32.empty()) {
        // Choose normalization based on channel settings
        float mn = node->cachedMin, mx = node->cachedMax;
        switch (ch.norm.policy) {
          case NormalizationPolicy::Auto:
            // already set via cachedMin/Max
            break;
          case NormalizationPolicy::Fixed:
            mn = ch.norm.minVal; mx = ch.norm.maxVal; break;
          case NormalizationPolicy::Percentile: {
            auto mm = percentiles(node->dataF32.data(), w, h, w*int(sizeof(float)), ch.norm.lowPct, ch.norm.highPct);
            mn = mm.first; mx = mm.second; break; }
        }
        uploadFloatToTexture(it->second.tex, node->dataF32.data(), w, h, mn, mx);
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

    // Pixel query tooltip in window title (MVP): show for first channel under cursor
    int mx, my; SDL_GetMouseState(&mx, &my);
    std::string under;
    SDL_FRect rect{};
    for (const auto &kv : g_lastRects) {
      const auto &r = kv.second;
      const float fx = static_cast<float>(mx);
      const float fy = static_cast<float>(my);
      if (fx >= r.x && fx < r.x + r.w && fy >= r.y && fy < r.y + r.h) { under = kv.first; rect = r; break; }
    }
    if (!under.empty()) {
      auto &ch = g_channels.getOrCreateChannel(under);
      if (ch.baseImage2D) {
        auto *node = static_cast<Image2DNode*>(ch.baseImage2D.get());
        const int w = node->width, h = node->height;
        // Map mouse to image coords
        const float sx = ch.view2D.scaleVector()[0];
        const float sy = ch.view2D.scaleVector()[1];
        const float tx = ch.view2D.translation()[0];
        const float ty = ch.view2D.translation()[1];
        int ix = int((static_cast<float>(mx) - tx) / (sx != 0.0f ? sx : 1.0f));
        int iy = int((static_cast<float>(my) - ty) / (sy != 0.0f ? sy : 1.0f));
        float orig = 0.0f, disp = 0.0f;
        if (ix >= 0 && iy >= 0 && ix < w && iy < h) {
          const int idx = iy*w + ix;
          if (node->format == Image2DFormat::U8 && !node->dataU8.empty()) {
            orig = static_cast<float>(node->dataU8[static_cast<size_t>(idx)]) / 255.0f;
            disp = orig;
          } else if (node->format == Image2DFormat::F32 && !node->dataF32.empty()) {
            orig = node->dataF32[static_cast<size_t>(idx)];
            float mn = node->cachedMin, mxv = node->cachedMax;
            switch (ch.norm.policy) {
              case NormalizationPolicy::Auto: break;
              case NormalizationPolicy::Fixed: mn = ch.norm.minVal; mxv = ch.norm.maxVal; break;
              case NormalizationPolicy::Percentile: {
                auto mm = percentiles(node->dataF32.data(), w, h, w*int(sizeof(float)), ch.norm.lowPct, ch.norm.highPct);
                mn = mm.first; mxv = mm.second; break; }
            }
            if (mxv <= mn) mxv = mn + 1.0f;
            disp = (orig - mn) / (mxv - mn);
            if (disp < 0.0f) disp = 0.0f;
            if (disp > 1.0f) disp = 1.0f;
          }
          char title[256];
          std::snprintf(title, sizeof(title), "Ravl2 Debug Display — %s (%d,%d) orig=%.6g disp=%.4f", under.c_str(), ix, iy, static_cast<double>(orig), static_cast<double>(disp));
          SDL_SetWindowTitle(g_window, title);
        }
      }
    } else {
      SDL_SetWindowTitle(g_window, "Ravl2 Debug Display");
    }

    // Note: SDL_RenderPresent is called after ImGui rendering in guiThreadMain.
#else
    (void)0;
#endif
  }

  void guiThreadMain(std::stop_token st)
  {
    SPDLOG_INFO("DebugDisplay: GUI thread started (SDL window+renderer)");
    if (!initSDLAndWindow()) {
      SPDLOG_ERROR("DebugDisplay: failed to initialize SDL window/renderer; GUI thread exiting");
      return;
    }

    while(!st.stop_requested()) {
      // Wait for events with timeout to keep CPU low when idle
      SDL_WaitEventTimeout(nullptr, 33); // ~30 FPS heartbeat
      processEvents(st);

      // Drain at most N commands per tick to bound work; mark invalidated when changes occur
      int drained = 0;
      std::shared_ptr<IRenderCommand> cmd;
      while (drained < 64 && g_queue.tryPop(cmd)) {
        if (cmd) { cmd->apply(g_channels); g_invalidated.store(true, std::memory_order_release); }
        ++drained;
      }

      // Begin ImGui frame (always build UI)
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
      if (g_imguiInitialized && g_bgfx.initialized()) {
        int mx=0,my=0; uint32_t mstate = SDL_GetMouseState(&mx, &my);
        // SDL mouse state already tracked for buttons/scroll
        uint16_t fbw = 0, fbh = 0;
        int winW=0, winH=0;
        SDL_GetWindowSize(g_window, &winW, &winH);
        fbw = static_cast<uint16_t>(winW);
        fbh = static_cast<uint16_t>(winH);
        imguiBeginFrame(mx, my, g_mouseButtons, g_scroll, fbw, fbh);
        g_scroll = 0; // consume scroll

        // Dockspace over main viewport
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        // Channel windows with images
        g_lastRects.clear();
        RenderContext rc{}; rc.framebufferWidth = fbw; rc.framebufferHeight = fbh;
        g_channels.forEachChannel([&](ChannelState &ch){
          if (!ImGui::Begin(ch.name.c_str())) { ImGui::End(); return; }
          if (ch.baseImage2D) {
            auto *node = static_cast<Image2DNode*>(ch.baseImage2D.get());
            node->prepare(rc);
#if defined(RAVL2_WITH_BGFX)
            if (node->width > 0 && node->height > 0 && node->textureHandleIdx != UINT16_MAX) {
              bgfx::TextureHandle thdl{node->textureHandleIdx};
              const float sx = ch.view2D.scaleVector()[0];
              const float sy = ch.view2D.scaleVector()[1];
              const float tx = ch.view2D.translation()[0];
              const float ty = ch.view2D.translation()[1];
              // Compute position in screen space
              ImVec2 winPos = ImGui::GetCursorScreenPos();
              ImVec2 pos = ImVec2(winPos.x + tx, winPos.y + ty);
              ImVec2 size = ImVec2(static_cast<float>(node->width) * sx, static_cast<float>(node->height) * sy);
              // Set cursor and draw
              ImGui::SetCursorScreenPos(pos);
              ImGui::Image(thdl, size);
              // Update hit-test rect in screen space
              SDL_FRect r{ pos.x, pos.y, size.x, size.y };
              g_lastRects[ch.name] = r;
            }
#endif
          }
          ImGui::End();
        });

        // Pixel query: update window title using screen-space g_lastRects
        {
          int mx, my; SDL_GetMouseState(&mx, &my);
          std::string under;
          SDL_FRect rect{};
          for (const auto &kv : g_lastRects) {
            const auto &r = kv.second;
            const float fx = static_cast<float>(mx);
            const float fy = static_cast<float>(my);
            if (fx >= r.x && fx < r.x + r.w && fy >= r.y && fy < r.y + r.h) { under = kv.first; rect = r; break; }
          }
          if (!under.empty()) {
            auto &ch = g_channels.getOrCreateChannel(under);
            if (ch.baseImage2D) {
              auto *node = static_cast<Image2DNode*>(ch.baseImage2D.get());
              const int w = node->width, h = node->height;
              const float sx = ch.view2D.scaleVector()[0];
              const float sy = ch.view2D.scaleVector()[1];
              const float tx = ch.view2D.translation()[0];
              const float ty = ch.view2D.translation()[1];
              int ix = int((static_cast<float>(mx) - tx - rect.x + rect.x - rect.x + (0.0f)) / (sx != 0.0f ? sx : 1.0f));
              int iy = int((static_cast<float>(my) - ty - rect.y + rect.y - rect.y + (0.0f)) / (sy != 0.0f ? sy : 1.0f));
              float orig = 0.0f, disp = 0.0f;
              if (ix >= 0 && iy >= 0 && ix < w && iy < h) {
                const int idx = iy*w + ix;
                if (node->format == Image2DFormat::U8 && !node->dataU8.empty()) {
                  orig = static_cast<float>(node->dataU8[static_cast<size_t>(idx)]) / 255.0f;
                  disp = orig;
                } else if (node->format == Image2DFormat::F32 && !node->dataF32.empty()) {
                  orig = node->dataF32[static_cast<size_t>(idx)];
                  float mn = node->cachedMin, mxv = node->cachedMax;
                  switch (ch.norm.policy) {
                    case NormalizationPolicy::Auto: break;
                    case NormalizationPolicy::Fixed: mn = ch.norm.minVal; mxv = ch.norm.maxVal; break;
                    case NormalizationPolicy::Percentile: {
                      auto mm = percentiles(node->dataF32.data(), w, h, w*int(sizeof(float)), ch.norm.lowPct, ch.norm.highPct);
                      mn = mm.first; mxv = mm.second; break; }
                  }
                  if (mxv <= mn) mxv = mn + 1.0f;
                  disp = (orig - mn) / (mxv - mn);
                  if (disp < 0.0f) disp = 0.0f;
                  if (disp > 1.0f) disp = 1.0f;
                }
                char title[256];
                std::snprintf(title, sizeof(title), "Ravl2 Debug Display — %s (%d,%d) orig=%.6g disp=%.4f", under.c_str(), ix, iy, static_cast<double>(orig), static_cast<double>(disp));
                SDL_SetWindowTitle(g_window, title);
              }
            }
          } else {
            SDL_SetWindowTitle(g_window, "Ravl2 Debug Display");
          }
        }

        imguiEndFrame();
      }
#elif defined(RAVL2_WITH_IMGUI)
      if (g_imguiInitialized) {
        ImGui_ImplSDL2_NewFrame();
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui::NewFrame();

        // Dockspace over main viewport
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        // Simple Channels window
        if (ImGui::Begin("Channels")) {
          g_channels.forEachChannel([](ChannelState &ch){
            ImGui::BulletText("%s", ch.name.c_str());
          });
        }
        ImGui::End();
      }
#endif

      // Only redraw image content when invalidated (SDL fallback path)
#if !defined(RAVL2_WITH_BGFX)
      if (g_invalidated.exchange(false, std::memory_order_acq_rel)) {
        renderAll();
      }
#endif

      // Render UI and present
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
      if (g_imguiInitialized && g_bgfx.initialized()) {
        // ImGui bgfx backend submits during imguiEndFrame via bgfx calls; nothing to do here
      }
#elif defined(RAVL2_WITH_IMGUI)
      if (g_imguiInitialized) {
        ImGui::Render();
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
      }
#endif
#if !defined(RAVL2_WITH_BGFX)
      SDL_RenderPresent(g_renderer);
#endif

      // Submit bgfx frame boundary
      g_bgfx.frame();
    }

    // Shutdown bgfx before SDL teardown
    g_bgfx.shutdown();

    shutdownSDL();
    SPDLOG_INFO("DebugDisplay: GUI thread exiting");
  }
}

void ensureStarted(const InitOptions &opts) {
  (void)opts;
  std::call_once(g_startOnce, []() {
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

} // namespace Ravl2::DebugDisplay
