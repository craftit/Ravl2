# Minimal finder for bx when fetched via CPM into _deps
# Provides:
#  BX_FOUND
#  BX_INCLUDE_DIRS

set(_BX_HINT_DIR "${CMAKE_BINARY_DIR}/_deps/bx-src" CACHE PATH "Hint path to bx sources")
set(BX_INCLUDE_DIRS "${_BX_HINT_DIR}/include")

if(TARGET bx)
  set(BX_FOUND TRUE)
else()
  if(EXISTS "${BX_INCLUDE_DIRS}/bx/bx.h")
    set(BX_FOUND TRUE)
    add_library(bx INTERFACE IMPORTED)
    set_property(TARGET bx PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${BX_INCLUDE_DIRS}")
  else()
    set(BX_FOUND FALSE)
  endif()
endif()

mark_as_advanced(BX_INCLUDE_DIRS)
