//! Logging configuration for Ravl2
//!
//! @file
//! @author GitHub Copilot
//! @date September 12, 2025
//!
//! @copyright
//! Copyright (c) React AI Ltd. All rights reserved.

#ifndef RAVL2_LOGGING_HH
#define RAVL2_LOGGING_HH

#include <spdlog/spdlog.h>

namespace Ravl2 {

//! Initialize logging with appropriate settings based on build configuration
//! In debug builds, this will set the log level to debug by default
//! In release builds, this will set the log level to info by default
inline void initializeLogging(spdlog::level::level_enum overrideLevel = spdlog::level::level_enum(-1)) {
  // If override level is specified, use it
  if (overrideLevel != spdlog::level::level_enum(-1)) {
    spdlog::set_level(overrideLevel);
    return;
  }

#ifdef NDEBUG
  // Release mode: set to info level by default
  spdlog::set_level(spdlog::level::info);
#else
  // Debug mode: set to debug level by default
  spdlog::set_level(spdlog::level::debug);
#endif
}

} // namespace Ravl2

#endif // RAVL2_LOGGING_HH
