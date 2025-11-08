# Minimal finder for bgfx when fetched via CPM into _deps

set(_BGFX_HINT_DIR "${CMAKE_BINARY_DIR}/_deps/bgfx-src" CACHE PATH "Hint path to bgfx sources")

set(BGFX_INCLUDE_DIRS "${_BGFX_HINT_DIR}/include")

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

mark_as_advanced(BGFX_INCLUDE_DIRS)
