# Ravl2 Debug Display (scaffold)

add_library(Ravl2Display
    DebugDisplay.cc DebugDisplay.hh
    Backends/BGFXContext.cc Backends/BGFXContext.hh
    IRenderCommand.hh ISceneNode.hh Channel.hh
    RenderContext.hh Normalization.hh
    Image2DNode.cc Image2DNode.hh
    Commands/SetBaseImage2D.cc Commands/SetBaseImage2D.hh
    Adapters/Array2DAdapters.cc
)

add_library(ravl2::Ravl2Display ALIAS Ravl2Display)


# Public headers are provided via include directories; avoid FILE_SET to keep CMake 3.21 compatibility.
# Headers:
#  - src/Ravl2/Display/DebugDisplay.hh
#  - src/Ravl2/Display/Channel.hh

# Link against core options/warnings and spdlog for logging
find_package(spdlog REQUIRED)

# SDL2 is required when the display stack is enabled
find_package(SDL2 REQUIRED)

target_link_libraries(Ravl2Display
  PRIVATE RAVL2_options RAVL2_warnings
  PUBLIC spdlog::spdlog SDL2::SDL2 Ravl2Core Ravl2Geometry
)

set_target_properties(Ravl2Display PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
  VISIBILITY_INLINES_HIDDEN YES
)

target_include_directories(Ravl2Display
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Sample executable that exercises the new DebugDisplay interface by loading an RGB image
# and saving it to the @debug channel. Built only when the Display stack is enabled.
add_executable(doDisplay
        doDisplay.cc
)

target_link_libraries(doDisplay
  PRIVATE
    Ravl2Display
    Ravl2Core
)

target_compile_definitions(doDisplay PRIVATE -DRAVL_SOURCE_DIR="${PROJECT_SOURCE_DIR}" -DRAVL_BINARY_DIR="${PROJECT_BINARY_DIR}" -DRAVL_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

set_target_properties(doDisplay PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
