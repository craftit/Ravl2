#pragma once

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>
#include "Ravl2/EntryPnt.hh"

namespace Ravl2::DebugDisplay {

//! Initialise the display subsystem. Must be called on the main thread.
//! On macOS, this initialises SDL which must happen on the main thread.
void initDisplay();

//! Optional: disable window/thread creation for tests or headless runs.
//! When enabled, ensureStarted() will not create an SDL window nor a GUI thread.
//! Routing and command enqueue remain available, but no frames are rendered.
//! Prefer `setHeadless` for general use; this alias remains for test code.
void setHeadlessForTests(bool on) noexcept;

//! Enable/disable headless mode for the process.
//! When true, no window or GUI thread will be started. Safe to call anytime
//! before or after `ensureStarted()`; if called before, startup will be headless.
void setHeadless(bool on) noexcept;

//! Query whether headless mode is enabled.
bool isHeadless() noexcept;


struct IRenderCommand; // fwd decl

//! Initialisation options for the debug display subsystem.
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
std::expected<void, std::string> enqueue(std::shared_ptr<IRenderCommand> command);


} // namespace Ravl2::DebugDisplay

