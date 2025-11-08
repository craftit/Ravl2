# Minimal finder for bimg when fetched via CPM into _deps
# Provides:
#  BIMG_FOUND
#  BIMG_INCLUDE_DIRS

set(_BIMG_HINT_DIR "${CMAKE_BINARY_DIR}/_deps/bimg-src" CACHE PATH "Hint path to bimg sources")
set(BIMG_INCLUDE_DIRS "${_BIMG_HINT_DIR}/include")

if(TARGET bimg)
  set(BIMG_FOUND TRUE)
else()
  if(EXISTS "${BIMG_INCLUDE_DIRS}/bimg/bimg.h")
    set(BIMG_FOUND TRUE)
    add_library(bimg INTERFACE IMPORTED)
    set_property(TARGET bimg PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${BIMG_INCLUDE_DIRS}")
  else()
    set(BIMG_FOUND FALSE)
  endif()
endif()

mark_as_advanced(BIMG_INCLUDE_DIRS)
