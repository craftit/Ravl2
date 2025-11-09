# Minimal finder for bgfx when fetched via CPM into _deps

# Common locations used by CPM packages:
#  - bgfx.cmake layout: <build>/_deps/bgfx.cmake-src/bgfx
#  - direct bgfx fetch: <build>/_deps/bgfx-src
set(_BGFX_HINT_DIR         "${CMAKE_BINARY_DIR}/_deps/bgfx-src"                   CACHE PATH "Hint path to bgfx sources (legacy)")
set(_BGFX_CMAKE_HINT_DIR   "${CMAKE_BINARY_DIR}/_deps/bgfx.cmake-src/bgfx"       CACHE PATH "Hint path to bgfx sources (bgfx.cmake)")

# Public include dir for C++ side
if(EXISTS "${_BGFX_CMAKE_HINT_DIR}/include")
  set(BGFX_INCLUDE_DIRS "${_BGFX_CMAKE_HINT_DIR}/include")
elseif(EXISTS "${_BGFX_HINT_DIR}/include")
  set(BGFX_INCLUDE_DIRS "${_BGFX_HINT_DIR}/include")
else()
  set(BGFX_INCLUDE_DIRS "${_BGFX_HINT_DIR}/include")
endif()

# Shader include dir contains bgfx_shader.sh under <bgfx_root>/src
set(BGFX_SHADER_INCLUDE_DIR "")
foreach(_root "${_BGFX_CMAKE_HINT_DIR}" "${_BGFX_HINT_DIR}")
  if(EXISTS "${_root}/src/bgfx_shader.sh")
    set(BGFX_SHADER_INCLUDE_DIR "${_root}/src")
    break()
  endif()
endforeach()

# If a target named bgfx already exists (provided by a package), mark as found
if(TARGET bgfx)
  set(BGFX_FOUND TRUE)
else()
  # Leave BGFX_FOUND as FALSE; linking will be handled by caller if not present
  if(EXISTS "${BGFX_INCLUDE_DIRS}/bgfx/bgfx.h")
    set(BGFX_FOUND TRUE)
  else()
    set(BGFX_FOUND FALSE)
  endif()
endif()

# Export cache variables for consumers
set(BGFX_INCLUDE_DIRS "${BGFX_INCLUDE_DIRS}" CACHE PATH "bgfx public include directory")
set(BGFX_SHADER_INCLUDE_DIR "${BGFX_SHADER_INCLUDE_DIR}" CACHE PATH "bgfx shader include directory (contains bgfx_shader.sh)")

mark_as_advanced(BGFX_INCLUDE_DIRS BGFX_SHADER_INCLUDE_DIR _BGFX_HINT_DIR _BGFX_CMAKE_HINT_DIR)
