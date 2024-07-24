
include(GenerateExportHeader)

add_library (Ravl2Image
        EdgeSobel.cc EdgeSobel.hh
        BilinearInterpolation.hh
        ImagePyramid.hh
        Array2Sqr2Iter.hh
        Array2Sqr2Iter2.hh
        DrawFrame.hh
        DrawCircle.hh
        DrawLine.hh
        DrawCross.hh
        Array2dPolygon2dIter.hh
        Corner.cc Corner.hh
        CornerDetectorSusan.cc CornerDetectorSusan.hh
        Segmentation/FloodRegion.cc
)

add_library(ravl2::Ravl2Image ALIAS Ravl2Image)

target_link_libraries(Ravl2Image PRIVATE ravl2_options ravl2_warnings)
target_link_libraries(Ravl2Image PUBLIC Ravl2Core Ravl2Geometry)

target_include_directories(Ravl2Image ${WARNING_GUARD} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

target_compile_features(Ravl2Image PUBLIC cxx_std_20)

set_target_properties(
        Ravl2Image
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(Ravl2Image EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/ravl2/Ravl2Core_export.hpp)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(Ravl2Image PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()


