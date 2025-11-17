


include(GenerateExportHeader)

add_library(Ravl2ImageIO
    JpegTurboImageIO.hh
    JpegTurboImageIO.cc
)

add_library(ravl2::Ravl2ImageIO ALIAS Ravl2ImageIO)

target_link_libraries(Ravl2ImageIO PRIVATE RAVL2_options RAVL2_warnings)
target_link_libraries(Ravl2ImageIO PUBLIC Ravl2Core)

target_include_directories(Ravl2ImageIO ${WARNING_GUARD} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
)

target_compile_features(Ravl2ImageIO PUBLIC cxx_std_23)

# Optional JPEG (libjpeg/libjpeg-turbo)
find_package(JPEG QUIET)
if(JPEG_FOUND)
  target_link_libraries(Ravl2ImageIO PRIVATE JPEG::JPEG)
  target_compile_definitions(Ravl2ImageIO PRIVATE RAVL2_HAVE_JPEG=1)
  target_include_directories(Ravl2ImageIO PRIVATE ${JPEG_INCLUDE_DIR})
else()
  message(STATUS "libjpeg not found; JPEGTurbo loader will remain a stub (no decode)")
endif()

set_target_properties(
        Ravl2ImageIO
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(Ravl2ImageIO EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/ravl2/Ravl2ImageIO_export.hpp)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(Ravl2ImageIO PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()
