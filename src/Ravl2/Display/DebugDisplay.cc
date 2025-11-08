#include "Ravl2/Display/DebugDisplay.hh"
#include "Ravl2/Display/IRenderCommand.hh"
#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/Image2DNode.hh"

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
  ThreadedQueue<std::unique_ptr<IRenderCommand>> g_queue{128};

  // Channel registry lives on the GUI thread
  ChannelRegistry g_channels;

  // Background thread (GUI thread)
  std::unique_ptr<std::jthread> g_guiThread;

  // SDL window and state
  SDL_Window* g_window = nullptr;
  SDL_Renderer* g_renderer = nullptr;
  std::atomic_bool g_invalidated{true};

  // Per-channel texture cache
  struct TextureEntry {
    SDL_Texture* tex = nullptr;
    int w = 0;
    int h = 0;
  };
  std::unordered_map<std::string, TextureEntry> g_textures;

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

    SPDLOG_INFO("DebugDisplay: SDL window+renderer created ({}x{})", 1280, 720);
    return true;
  }

  void shutdownSDL()
  {
    for (auto &kv : g_textures) {
      if (kv.second.tex) SDL_DestroyTexture(kv.second.tex);
    }
    g_textures.clear();

    if (g_renderer) { SDL_DestroyRenderer(g_renderer); g_renderer = nullptr; }
    if (g_window) { SDL_DestroyWindow(g_window); g_window = nullptr; }
    if (SDL_WasInit(0)) { SDL_Quit(); }
  }

  void processEvents(std::stop_token st)
  {
    (void)st; // unused for now; reserved for future stop-aware processing
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        // Request stop on quit
        if (g_guiThread) { g_guiThread->request_stop(); }
        return;
      }
      if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
            e.window.event == SDL_WINDOWEVENT_RESIZED) {
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
      // Using RGB24 texture; we'll expand grayscale to RGB when uploading.
      entry.tex = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, w, h);
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
    // Expand to RGB24
    auto *dst = static_cast<uint8_t*>(pixels);
    for (int y=0; y<h; ++y) {
      uint8_t* row = dst + y * pitch;
      const uint8_t* srcRow = gray + y * w;
      for (int x=0; x<w; ++x) {
        uint8_t v = srcRow[x];
        row[x*3+0] = v; row[x*3+1] = v; row[x*3+2] = v;
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
    if (!g_renderer) return;
    SDL_SetRenderDrawColor(g_renderer, 16, 16, 24, 255);
    SDL_RenderClear(g_renderer);

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
        uploadFloatToTexture(it->second.tex, node->dataF32.data(), w, h, node->cachedMin, node->cachedMax);
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

      SDL_RenderCopyF(g_renderer, it->second.tex, nullptr, &dst);
    });

    SDL_RenderPresent(g_renderer);
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
      std::unique_ptr<IRenderCommand> cmd;
      while (drained < 64 && g_queue.tryPop(cmd)) {
        if (cmd) { cmd->apply(g_channels); g_invalidated.store(true, std::memory_order_release); }
        ++drained;
      }

      if (g_invalidated.exchange(false, std::memory_order_acq_rel)) {
        renderAll();
      }
    }

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

std::expected<void, std::string> enqueue(std::unique_ptr<IRenderCommand> command)
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
  std::unique_ptr<IRenderCommand> cmd;
  const bool hasClearCtrl = (!controls.empty() && controls.find(":Clear") != std::string_view::npos);
  if (hasClearCtrl) {
    cmd = std::make_unique<Commands::ClearChannelCommand>(std::string{channel});
    return enqueue(std::move(cmd));
  }
  // No recognized control; accept as no-op for backward compatibility
  return {};
}

} // namespace Ravl2::DebugDisplay
