# Ravl2 Debug Display (scaffold)

# Ensure local Find*.cmake modules are discoverable
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

add_library(Ravl2Display
    DebugDisplay.cc DebugDisplay.hh
    Backends/BGFXContext.cc Backends/BGFXContext.hh
    Backends/ImguiBgfxBridge.cc Backends/ImguiBgfxBridge.hh
    Ui/Dockspace.cc Ui/Dockspace.hh
    Ui/ControlsPanel.cc Ui/ControlsPanel.hh
    Ui/ChannelWindows.cc Ui/ChannelWindows.hh
    Ui/StatusBar.cc Ui/StatusBar.hh
    Ui/Plots.cc Ui/Plots.hh
    InputController2D.cc InputController2D.hh
    PixelInspector2D.cc PixelInspector2D.hh
    Overlays/OverlayRenderer2D.cc Overlays/OverlayRenderer2D.hh
    IRenderCommand.hh ISceneNode.hh Channel.hh
    RenderContext.hh Normalization.hh
    Image2DNode.cc Image2DNode.hh
    Commands/SetBaseImage2D.cc Commands/SetBaseImage2D.hh
    Commands/SetNormalization2D.hh Commands/SetNormalization2D.cc
    Commands/AddPolylineOverlay2D.cc Commands/AddPolylineOverlay2D.hh
    Adapters/RenderCommandSink.cc
)

# On macOS, compile BGFXContext.cc as Objective-C++ to use Cocoa framework
if(APPLE)
  set_source_files_properties(Backends/BGFXContext.cc PROPERTIES
    COMPILE_FLAGS "-x objective-c++"
  )
endif()

add_library(ravl2::Ravl2Display ALIAS Ravl2Display)


# Public headers are provided via include directories; avoid FILE_SET to keep CMake 3.21 compatibility.
# Headers:
#  - src/Ravl2/Display/DebugDisplay.hh
#  - src/Ravl2/Display/Channel.hh

# Link against core options/warnings and spdlog for logging
find_package(spdlog REQUIRED)

# SDL2 is required when the display stack is enabled
find_package(SDL2 REQUIRED)

include(SetupImGUI)

target_link_libraries(Ravl2Display
  PRIVATE RAVL2_options RAVL2_warnings
  PUBLIC spdlog::spdlog SDL2::SDL2 Ravl2Core Ravl2Geometry
)

# On macOS, link required frameworks for Metal and Cocoa
if(APPLE)
  target_link_libraries(Ravl2Display PRIVATE
    "-framework Metal"
    "-framework QuartzCore"
    "-framework Cocoa"
  )
endif()

# ImGui integration (ensure finder runs and create/consume target)
find_package(imgui QUIET)
message(STATUS "IMGUI_FOUND= ${IMGUI_FOUND}")

if(TARGET imgui)
  message(STATUS "Ravl2Display: integrating Dear ImGui")
  target_link_libraries(Ravl2Display PRIVATE imgui)
  target_compile_definitions(Ravl2Display PRIVATE RAVL2_WITH_IMGUI=1)
else()
  message(STATUS "Ravl2Display: no Dear ImGui available")
endif()

# Link bgfx stack if available (targets created by CPM in Dependencies.cmake)
if(TARGET bgfx)
  target_link_libraries(Ravl2Display PRIVATE bgfx)
  target_compile_definitions(Ravl2Display PRIVATE RAVL2_WITH_BGFX=1)
  message(STATUS "Ravl2Display: linking bgfx")
endif()
if(TARGET bimg)
  target_link_libraries(Ravl2Display PRIVATE bimg)
  message(STATUS "Ravl2Display: linking bimg")
endif()
if(TARGET bx)
  target_link_libraries(Ravl2Display PRIVATE bx)
  message(STATUS "Ravl2Display: linking bx")
endif()

# Try to load helper variables from Findbgfx.cmake (BGFX_SHADER_INCLUDE_DIR)
find_package(bgfx QUIET)

# Compile bgfx shaders for non-ImGui textured quad (GLSL + SPIR-V)
if(TARGET shaderc)
  set(RAVL2_SHADER_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/shaders)
  set(RAVL2_SHADER_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
  file(MAKE_DIRECTORY ${RAVL2_SHADER_BIN_DIR})

  set(VARYING_DEF ${RAVL2_SHADER_SRC_DIR}/varying.def.sc)
  set(VS_SRC ${RAVL2_SHADER_SRC_DIR}/vs_image.sc)
  set(FS_SRC ${RAVL2_SHADER_SRC_DIR}/fs_image.sc)

  set(VS_GLSL ${RAVL2_SHADER_BIN_DIR}/vs_image.glsl.bin)
  set(FS_GLSL ${RAVL2_SHADER_BIN_DIR}/fs_image.glsl.bin)
  set(VS_SPV  ${RAVL2_SHADER_BIN_DIR}/vs_image.spv.bin)
  set(FS_SPV  ${RAVL2_SHADER_BIN_DIR}/fs_image.spv.bin)

  # Determine bgfx shader include directory containing bgfx_shader.sh.
  # Try to locate it robustly via find_file among common locations created by bgfx.cmake.
  if(TARGET shaderc)
    if(NOT DEFINED BGFX_SHADER_INCLUDE_DIR)
      find_file(BGFX_SHADER_SH
        NAMES bgfx_shader.sh
        HINTS
          ${PROJECT_BINARY_DIR}/_deps/bgfx.cmake-src/bgfx/src
          ${CMAKE_BINARY_DIR}/_deps/bgfx.cmake-src/bgfx/src
          ${PROJECT_SOURCE_DIR}/cmake-build-debug/_deps/bgfx.cmake-src/bgfx/src
          ${PROJECT_SOURCE_DIR}/cmake-build-release/_deps/bgfx.cmake-src/bgfx/src
          ${PROJECT_SOURCE_DIR}/_deps/bgfx.cmake-src/bgfx/src
        NO_DEFAULT_PATH
      )
      if(BGFX_SHADER_SH)
        get_filename_component(BGFX_SHADER_INCLUDE_DIR "${BGFX_SHADER_SH}" DIRECTORY)
        message(STATUS "bgfx shader include dir: ${BGFX_SHADER_INCLUDE_DIR}")
      else()
        message(STATUS "bgfx_shader.sh not found; you can set -DBGFX_SHADER_INCLUDE_DIR=/path/to/bgfx/src to enable bgfx shader includes")
      endif()
    endif()
  endif()

  # Compose optional include args for bgfx shader headers.
  set(BGFX_SHADER_INC_ARGS)
  if(BGFX_SHADER_INCLUDE_DIR)
    list(APPEND BGFX_SHADER_INC_ARGS -i ${BGFX_SHADER_INCLUDE_DIR})
  endif()

  add_custom_command(
    OUTPUT ${VS_GLSL}
    COMMAND $<TARGET_FILE:shaderc> -f ${VS_SRC} -o ${VS_GLSL} --type vertex --platform linux --profile 430 --varyingdef ${VARYING_DEF} -i ${RAVL2_SHADER_SRC_DIR} ${BGFX_SHADER_INC_ARGS}
    DEPENDS ${VS_SRC} ${VARYING_DEF}
    VERBATIM
  )
  add_custom_command(
    OUTPUT ${FS_GLSL}
    COMMAND $<TARGET_FILE:shaderc> -f ${FS_SRC} -o ${FS_GLSL} --type fragment --platform linux --profile 430 --varyingdef ${VARYING_DEF} -i ${RAVL2_SHADER_SRC_DIR} ${BGFX_SHADER_INC_ARGS}
    DEPENDS ${FS_SRC} ${VARYING_DEF}
    VERBATIM
  )
  add_custom_command(
    OUTPUT ${VS_SPV}
    COMMAND $<TARGET_FILE:shaderc> -f ${VS_SRC} -o ${VS_SPV} --type vertex --platform linux --profile spirv --varyingdef ${VARYING_DEF} -i ${RAVL2_SHADER_SRC_DIR} ${BGFX_SHADER_INC_ARGS}
    DEPENDS ${VS_SRC} ${VARYING_DEF}
    VERBATIM
  )
  add_custom_command(
    OUTPUT ${FS_SPV}
    COMMAND $<TARGET_FILE:shaderc> -f ${FS_SRC} -o ${FS_SPV} --type fragment --platform linux --profile spirv --varyingdef ${VARYING_DEF} -i ${RAVL2_SHADER_SRC_DIR} ${BGFX_SHADER_INC_ARGS}
    DEPENDS ${FS_SRC} ${VARYING_DEF}
    VERBATIM
  )

  add_custom_target(Ravl2DisplayShaders
    DEPENDS ${VS_GLSL} ${FS_GLSL} ${VS_SPV} ${FS_SPV}
  )
  add_dependencies(Ravl2Display Ravl2DisplayShaders)
  target_compile_definitions(Ravl2Display PRIVATE RAVL2_SHADER_DIR="${RAVL2_SHADER_BIN_DIR}")
else()
  message(STATUS "shaderc target not available; bgfx shaders will not be compiled. SDL fallback will be used if missing.")
endif()

# ImGui backend: always prefer bgfx backend (vendored) when both imgui and bgfx are available.
# We do not use the SDL_Renderer backend path; this keeps the build simple and avoids extra dependencies.
if(TARGET imgui AND TARGET bgfx)
  message(STATUS "Ravl2Display: using ImGui bgfx backend ")
  add_library(bgfx_imgui STATIC
          ${CMAKE_CURRENT_LIST_DIR}/bgfx_imgui/ImGUI/imgui.cpp
          ${CMAKE_CURRENT_LIST_DIR}/bgfx_imgui/ImGUI/dock.cc
# ${CMAKE_CURRENT_LIST_DIR}/bgfx_imgui/bgfx_utils.cpp
  )

  target_include_directories(bgfx_imgui PUBLIC ${bgfx_SOURCE_DIR}/bgfx/3rdparty)
  target_link_libraries(bgfx_imgui PUBLIC imgui bimg bx bgfx)
  target_link_libraries(Ravl2Display PUBLIC bgfx_imgui)
endif()

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
    Ravl2OpenCV

)

target_compile_definitions(doDisplay PRIVATE -DRAVL_SOURCE_DIR="${PROJECT_SOURCE_DIR}" -DRAVL_BINARY_DIR="${PROJECT_BINARY_DIR}" -DRAVL_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

set_target_properties(doDisplay PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
