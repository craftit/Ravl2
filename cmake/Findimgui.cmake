# Minimal finder for Dear ImGui when fetched via CPM into _deps
# Provides:
#  IMGUI_FOUND
#  IMGUI_INCLUDE_DIRS
#  IMGUI_BACKENDS_DIR
#  imgui (target) if available

set(_IMGUI_HINT_DIR "${CMAKE_BINARY_DIR}/_deps/imgui-src" CACHE PATH "Hint path to imgui sources")

set(IMGUI_DIR "${_IMGUI_HINT_DIR}")
set(IMGUI_INCLUDE_DIRS "${IMGUI_DIR}" "${IMGUI_DIR}/backends")
set(IMGUI_BACKENDS_DIR "${IMGUI_DIR}/backends")

if(TARGET imgui)
  set(IMGUI_FOUND TRUE)
else()
  if(EXISTS "${_IMGUI_HINT_DIR}/imgui.h")
    set(IMGUI_FOUND TRUE)
    # Optionally define an INTERFACE target for consumers
    add_library(imgui INTERFACE IMPORTED)
    set_property(TARGET imgui PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${_IMGUI_HINT_DIR};${_IMGUI_HINT_DIR}/backends")
  else()
    set(IMGUI_FOUND FALSE)
  endif()
endif()

mark_as_advanced(IMGUI_INCLUDE_DIRS IMGUI_BACKENDS_DIR)
