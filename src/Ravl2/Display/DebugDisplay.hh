#pragma once

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>

namespace Ravl2::DebugDisplay {

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
//! @param command Unique pointer to a command (will be moved into the queue).
//! @return std::expected<void, std::string> with error message on failure.
std::expected<void, std::string> enqueue(
    std::unique_ptr<IRenderCommand> command);

//! Deprecated shim: payload-based enqueue retained temporarily for migration.
[[deprecated("Use enqueue(unique_ptr<IRenderCommand>) instead")]]
std::expected<void, std::string> enqueue(
    std::string_view channel,
    std::type_index type,
    std::shared_ptr<const void> payload,
    uint32_t flags,
    std::string_view controls);

} // namespace Ravl2::DebugDisplay
