# Minimal finder for Dear ImGui when sources are present but no CMake target is exported.
# Provides:
#  IMGUI_FOUND
#  IMGUI_INCLUDE_DIRS
#  IMGUI_BACKENDS_DIR
#  imgui (target) — a STATIC library built from core sources when no target exists

set(_IMGUI_HINT_DIR "${CMAKE_BINARY_DIR}/_deps/imgui-src" CACHE PATH "Hint path to imgui sources")

# Allow manual override of IMGUI_DIR (e.g., vendored third_party/imgui)
if(NOT DEFINED IMGUI_DIR)
  set(IMGUI_DIR "${_IMGUI_HINT_DIR}")
endif()

set(IMGUI_INCLUDE_DIRS "${IMGUI_DIR}" "${IMGUI_DIR}/backends")
set(IMGUI_BACKENDS_DIR "${IMGUI_DIR}/backends")

message(STATUS "***** imgui dir is ${IMGUI_DIR} ")

if(TARGET imgui)
  message(STATUS "***** imgui already exists in ${IMGUI_DIR} ")
  set(IMGUI_FOUND TRUE)
else()
  message(STATUS "***** Looking for imgui in ${IMGUI_DIR} ")
  if(EXISTS "${IMGUI_DIR}/imgui.h")
    # Build a minimal static library with ImGui core sources.
    set(_IMGUI_SRC
      ${IMGUI_DIR}/imgui.cpp
      ${IMGUI_DIR}/imgui_draw.cpp
      ${IMGUI_DIR}/imgui_tables.cpp
      ${IMGUI_DIR}/imgui_widgets.cpp
      # Demo is optional but useful during development; comment out to reduce size.
      ${IMGUI_DIR}/imgui_demo.cpp
    )
    add_library(imgui STATIC ${_IMGUI_SRC})
    target_include_directories(imgui PUBLIC "${IMGUI_DIR}" "${IMGUI_DIR}/backends")
    # Relax a few warnings for 3rd-party code if your project treats warnings as errors.
    if(MSVC)
      target_compile_options(imgui PRIVATE /wd4996)
    else()
      target_compile_options(imgui PRIVATE -Wno-old-style-cast -Wno-zero-as-null-pointer-constant)
    endif()
    set(IMGUI_FOUND TRUE)
  else()
    set(IMGUI_FOUND FALSE)
  endif()
endif()

mark_as_advanced(IMGUI_INCLUDE_DIRS IMGUI_BACKENDS_DIR _IMGUI_HINT_DIR)
