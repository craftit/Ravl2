

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
        #CornerDetectorSusan.cc CornerDetectorSusan.hh
)

set_property(TARGET Ravl2Image PROPERTY CXX_STANDARD 20)
target_link_libraries(Ravl2Image PUBLIC Ravl2 Ravl2Geometry)

add_subdirectory(Segmentation)

add_executable(test_image ../../../test/test_image.cc ../../../test/test_draw.cc)
target_link_libraries(test_image Ravl2Image GTest::gtest_main)
add_test(NAME test_image COMMAND test_image)
