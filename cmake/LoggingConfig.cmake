# Configure spdlog active level based on build type
function(configure_logging_level)
  # In debug builds, set SPDLOG_ACTIVE_LEVEL to debug
  # In release builds, set SPDLOG_ACTIVE_LEVEL to info
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
    message(STATUS "Configuring SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG (Debug build)")
  else()
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
    message(STATUS "Configuring SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO (Release build)")
  endif()
endfunction()

