# Ravl2 Debug Display (scaffold)

# Ensure local Find*.cmake modules are discoverable
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

add_library(Ravl2Display
    DebugDisplay.cc DebugDisplay.hh
    Backends/BGFXContext.cc Backends/BGFXContext.hh
    IRenderCommand.hh ISceneNode.hh Channel.hh
    RenderContext.hh Normalization.hh
    Image2DNode.cc Image2DNode.hh
    Commands/SetBaseImage2D.cc Commands/SetBaseImage2D.hh
    Commands/SetNormalization2D.hh Commands/SetNormalization2D.cc
    Adapters/RenderCommandSink.cc
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

# ImGui integration
find_package(imgui QUIET)
if(IMGUI_FOUND)
  message(STATUS "Ravl2Display: integrating Dear ImGui")
  target_link_libraries(Ravl2Display PRIVATE imgui)
  target_compile_definitions(Ravl2Display PRIVATE RAVL2_WITH_IMGUI=1)
  if(IMGUI_INCLUDE_DIRS)
    target_include_directories(Ravl2Display PRIVATE ${IMGUI_INCLUDE_DIRS})
  endif()
endif()

# Link bgfx stack if available
find_package(bgfx QUIET)
if(BGFX_FOUND)
  if(TARGET bgfx)
    target_link_libraries(Ravl2Display PRIVATE bgfx)
    target_compile_definitions(Ravl2Display PRIVATE RAVL2_WITH_BGFX=1)
  endif()
  if(BGFX_INCLUDE_DIRS)
    target_include_directories(Ravl2Display PRIVATE ${BGFX_INCLUDE_DIRS})
  endif()
endif()
find_package(bimg QUIET)
if(BIMG_FOUND)
  target_link_libraries(Ravl2Display PRIVATE bimg)
  if(BIMG_INCLUDE_DIRS)
    target_include_directories(Ravl2Display PRIVATE ${BIMG_INCLUDE_DIRS})
  endif()
endif()
find_package(bx QUIET)
if(BX_FOUND)
  target_link_libraries(Ravl2Display PRIVATE bx)
  if(BX_INCLUDE_DIRS)
    target_include_directories(Ravl2Display PRIVATE ${BX_INCLUDE_DIRS})
  endif()
endif()

# Choose ImGui backend: use bgfx backend when bgfx is available; otherwise SDL_Renderer backend if provided via IMGUI_BACKENDS_DIR
if(IMGUI_FOUND AND BGFX_FOUND)
  message(STATUS "Ravl2Display: using ImGui bgfx backend (vendored)")
  target_sources(Ravl2Display PRIVATE
    bgfx_imgui/imgui.cpp
  )
  target_include_directories(Ravl2Display PRIVATE ${CMAKE_CURRENT_LIST_DIR}/bgfx_imgui)
elseif(IMGUI_FOUND AND DEFINED IMGUI_BACKENDS_DIR)
  message(STATUS "Ravl2Display: using ImGui SDL_Renderer backend")
  target_sources(Ravl2Display PRIVATE
    ${IMGUI_BACKENDS_DIR}/imgui_impl_sdl2.cpp
    ${IMGUI_BACKENDS_DIR}/imgui_impl_sdlrenderer.cpp
  )
  target_include_directories(Ravl2Display PRIVATE ${IMGUI_BACKENDS_DIR})
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
)

target_compile_definitions(doDisplay PRIVATE -DRAVL_SOURCE_DIR="${PROJECT_SOURCE_DIR}" -DRAVL_BINARY_DIR="${PROJECT_BINARY_DIR}" -DRAVL_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

set_target_properties(doDisplay PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
