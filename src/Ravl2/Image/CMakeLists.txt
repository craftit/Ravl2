
include(GenerateExportHeader)

add_library (Ravl2Image
        EdgeSobel.hh EdgeSobel.cc
        BilinearInterpolation.cc BilinearInterpolation.hh
        ImagePyramidC.hh
        Array2Sqr2Iter.hh
        Array2Sqr2Iter2.hh
        DrawFrame.hh
        DrawCircle.hh DrawCircle.cc
        DrawEllipse.hh
        DrawLine.hh
        DrawCross.hh
        DrawMask.hh DrawMask.cc
        DrawPolygon.hh DrawPolygon.cc
        Array2dPolygon2dIter.hh
        Corner.cc Corner.hh
        CornerDetectorSusan.cc CornerDetectorSusan.hh
        PeakDetector.cc PeakDetector.hh
        Matching.cc Matching.hh
        ImagePyramidC.cc ImagePyramidC.hh
        ImagePyramid.cc ImagePyramid.hh
        WarpScale.cc WarpScale.hh
        ImageExtend.cc ImageExtend.hh
        ZigZagIter.cc ZigZagIter.hh
        DCT2d.cc DCT2d.hh
        BitmapFont.cc BitmapFont.hh
        DrawText.cc DrawText.hh
        WarpAffine.hh WarpAffine.cc
        WarpProjective.hh
        Warp.cc
        Warp.hh
        #        MatchNormalisedCorrelation.cc MatchNormalisedCorrelation.hh
)

add_library(ravl2::Ravl2Image ALIAS Ravl2Image)

target_link_libraries(Ravl2Image PRIVATE RAVL2_options RAVL2_warnings)
target_link_libraries(Ravl2Image PUBLIC Ravl2Core Ravl2Geometry Ravl2Pixel)

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

add_subdirectory(Segmentation)

