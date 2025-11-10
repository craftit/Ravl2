#pragma once

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>

namespace Ravl2::DebugDisplay {

//! Initialize display subsystem. Must be called on the main thread.
//! On macOS, this initializes SDL which must happen on the main thread.
void initDisplay();

//! Optional: disable window/thread creation for tests or headless runs.
//! When enabled, ensureStarted() will not create an SDL window nor a GUI thread.
//! Routing and command enqueue remain available, but no frames are rendered.
void setHeadlessForTests(bool on) noexcept;

//! Run the main event loop (blocking). Only needed on macOS when using RAVL2_MAIN wrapper.
//! @param appMain Function pointer to the application's main logic
//! @param argc Command line argument count
//! @param argv Command line argument vector
//! @return Exit code from appMain
int runMainLoop(int (*appMain)(int, char**), int argc, char** argv);

struct IRenderCommand; // fwd decl

//! Initialization options for the debug display subsystem.
struct InitOptions {
  int maxFps = 60;            //!< Target max FPS for rendering.
  bool startHidden = false;   //!< Start minimized/hidden (if supported).
};

//! Ensure the debug display subsystem is started (lazy init).
//! Thread-safe. Safe to call multiple times.
void ensureStarted(const InitOptions &opts = {});

//! Enqueue a typed render command (command carries its own context, e.g., channel).
//! Thread-safe.
//! @param command Shared pointer to a command; may be referenced elsewhere until applied.
//! @return std::expected<void, std::string> with error message on failure.
std::expected<void, std::string> enqueue(
    std::shared_ptr<IRenderCommand> command);

//! Deprecated shim: payload-based enqueue retained temporarily for migration.
[[deprecated("Use enqueue(shared_ptr<IRenderCommand>) instead")]]
std::expected<void, std::string> enqueue(
    std::string_view channel,
    std::type_index type,
    std::shared_ptr<const void> payload,
    uint32_t flags,
    std::string_view controls);

} // namespace Ravl2::DebugDisplay

//! Macro to wrap main() for macOS compatibility.
//! On macOS, this ensures SDL runs on the actual main thread by moving user code to a background thread.
//! Usage: Replace `int main(int argc, char** argv)` with `int RAVL2_MAIN(int argc, char** argv)`
#ifdef __APPLE__
  #define RAVL2_MAIN \
    ravl2_app_main(int, char**); \
    int main(int argc, char** argv) { \
      return Ravl2::DebugDisplay::runMainLoop(ravl2_app_main, argc, argv); \
    } \
    int ravl2_app_main
#else
  #define RAVL2_MAIN main
#endif
