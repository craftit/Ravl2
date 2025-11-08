# Ravl2 Debug Display (scaffold)

add_library(ravl2_debug_display
    DebugDisplay.cc DebugDisplay.hh
    Backends/BGFXContext.cc Backends/BGFXContext.hh
    IRenderCommand.hh ISceneNode.hh Channel.hh
    RenderContext.hh Normalization.hh
    Image2DNode.cc Image2DNode.hh
    Commands/SetBaseImage2D.cc Commands/SetBaseImage2D.hh
    Adapters/Array2DAdapters.cc
)

add_library(ravl2::ravl2_debug_display ALIAS ravl2_debug_display)

# Public headers are provided via include directories; avoid FILE_SET to keep CMake 3.21 compatibility.
# Headers:
#  - src/Ravl2/Display/DebugDisplay.hh
#  - src/Ravl2/Display/Channel.hh

# Link against core options/warnings and spdlog for logging
find_package(spdlog REQUIRED)

# SDL2 is required when the display stack is enabled
find_package(SDL2 REQUIRED)

target_link_libraries(ravl2_debug_display
  PRIVATE RAVL2_options RAVL2_warnings
  PUBLIC spdlog::spdlog SDL2::SDL2 Ravl2Core
)

set_target_properties(ravl2_debug_display PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
  VISIBILITY_INLINES_HIDDEN YES
)

target_include_directories(ravl2_debug_display
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Sample executable that exercises the new DebugDisplay interface by loading an RGB image
# and saving it to the @debug channel. Built only when the Display stack is enabled.
add_executable(ravl2_debug_display_sample
  DebugDisplaySample.cc
)

target_link_libraries(ravl2_debug_display_sample
  PRIVATE
    ravl2_debug_display
    Ravl2Core
)

set_target_properties(ravl2_debug_display_sample PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
