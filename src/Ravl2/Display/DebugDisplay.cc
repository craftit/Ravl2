#include "Ravl2/Display/DebugDisplay.hh"
#include "Ravl2/Display/IRenderCommand.hh"
#include "Ravl2/Display/Channel.hh"

#include "Ravl2/ThreadedQueue.hh"

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
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
  std::atomic_bool g_invalidated{true};

  bool initSDLAndWindow()
  {
    if (SDL_WasInit(0) == 0) {
      if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SPDLOG_ERROR("DebugDisplay: SDL_Init failed: {}", SDL_GetError());
        return false;
        }
    }
    // Create a resizable, high-DPI aware window; renderer backend hookup to come later (bgfx)
    g_window = SDL_CreateWindow(
        "Ravl2 Debug Display",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
    if (!g_window) {
      SPDLOG_ERROR("DebugDisplay: SDL_CreateWindow failed: {}", SDL_GetError());
      return false;
    }
    SPDLOG_INFO("DebugDisplay: SDL window created ({}x{})", 1280, 720);
    return true;
  }

  void shutdownSDL()
  {
    if (g_window) {
      SDL_DestroyWindow(g_window);
      g_window = nullptr;
    }
    if (SDL_WasInit(0)) {
      SDL_Quit();
    }
  }

  void processEvents(std::stop_token st)
  {
    (void)st; // unused for now; reserved for future stop-aware processing
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        // Request stop on quit
        if (g_guiThread) {
          g_guiThread->request_stop();
        }
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

  void guiThreadMain(std::stop_token st)
  {
    SPDLOG_INFO("DebugDisplay: GUI thread started (SDL window, no rendering yet)");
    if (!initSDLAndWindow()) {
      SPDLOG_ERROR("DebugDisplay: failed to initialize SDL window; GUI thread exiting");
      return;
    }

    // Main loop: wait for events with timeout; drain command queue; render when invalidated (placeholder)
    while(!st.stop_requested()) {
      // Wait for events with timeout to keep CPU low when idle
      SDL_WaitEventTimeout(nullptr, 33); // ~30 FPS heartbeat
      processEvents(st);

      // Drain at most N commands per tick to bound work; mark invalidated when changes occur
      int drained = 0;
      std::unique_ptr<IRenderCommand> cmd;
      while (drained < 64 && g_queue.tryPop(cmd)) {
        if (cmd) {
          cmd->apply(g_channels);
          g_invalidated.store(true, std::memory_order_release);
        }
        ++drained;
      }

      // Placeholder for rendering: if invalidated, we would render via bgfx+ImGui in future step
      if (g_invalidated.exchange(false, std::memory_order_acq_rel)) {
        // For now just update window title with a simple tick to visualize activity
        static uint64_t tick = 0;
        ++tick;
        if ((tick % 30) == 0) {
          SDL_SetWindowTitle(g_window, "Ravl2 Debug Display (idle)");
        }
      }
    }

    shutdownSDL();
    SPDLOG_INFO("DebugDisplay: GUI thread exiting");
  }
}

void ensureStarted(const InitOptions &opts) {
  (void)opts;
  std::call_once(g_startOnce, []() {
    // Start background GUI thread (SDL window + future bgfx/imgui)
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
  if (!g_started.load(std::memory_order_acquire)) {
    ensureStarted({});
  }
  if (!command) {
    return std::unexpected(std::string{"DebugDisplay enqueue: null command"});
  }

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
