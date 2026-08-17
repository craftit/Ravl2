# GoPro GPMF module CMake file
# Created for RAVL2 GoPro metadata support

option(WITH_GPMF "Build with GoPro GPMF metadata support" ON)

if(NOT WITH_GPMF OR NOT HAVE_GPMF)
  message(STATUS "GoPro module disabled (GPMF parser not available)")
  return()
endif()

set(GOPRO_SOURCES
    GpmfTypes.cc GpmfTypes.hh
    GpmfParser.cc GpmfParser.hh
    GpmfUtilities.cc GpmfUtilities.hh
    VideoGpsSampler.cc VideoGpsSampler.hh
)

add_library(Ravl2GoPro ${GOPRO_SOURCES})
add_library(ravl2::Ravl2GoPro ALIAS Ravl2GoPro)

target_link_libraries(Ravl2GoPro
    PUBLIC
        ravl2::Ravl2Core
        ravl2::Ravl2Video
        # GpmfParser uses Ravl2::Quaternion<float>, whose out-of-line explicit
        # instantiation (extern template) lives in Ravl2Geometry; link it so
        # consumers (e.g. doEvalClusterCoherence) resolve those symbols.
        ravl2::Ravl2Geometry
    PRIVATE
        RAVL2_options
        RAVL2_warnings
)

# Link with gpmf-parser if available
if(TARGET gpmf::parser)
    target_link_libraries(Ravl2GoPro PRIVATE gpmf::parser)
elseif(TARGET GPMF_PARSER_LIB)
    target_link_libraries(Ravl2GoPro PRIVATE GPMF_PARSER_LIB)
    # Add include directory for GPMF headers
    if(GPMF_INCLUDE_DIR)
        target_include_directories(Ravl2GoPro PRIVATE ${GPMF_INCLUDE_DIR})
    endif()
else()
    message(WARNING "GPMF parser target not found")
endif()

target_include_directories(Ravl2GoPro
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
)

# Set installation targets for the library
install(TARGETS Ravl2GoPro
    EXPORT Ravl2Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install header files
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/Ravl2/GoPro
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Ravl2
    FILES_MATCHING PATTERN "*.hh"
)
