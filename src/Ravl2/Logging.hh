//! Logging configuration for Ravl2

#pragma once

#include <spdlog/spdlog.h>

namespace Ravl2 {

  //! Initialise logging with appropriate settings based on build configuration
  //! In debug builds, this will set the log level to debug by default
  //! In release builds, this will set the log level to info by default
  void initializeLogging();

  //! Initialise logging with appropriate settings based on build configuration
  //! In debug builds, this will set the log level to debug by default
  //! In release builds, this will set the log level to info by default
  void initializeLogging(spdlog::level::level_enum overrideLevel);

} // namespace Ravl2

