

add_library (Ravl2Image
        EdgeSobel.cc EdgeSobel.hh
        BilinearInterpolation.hh
        ImagePyramid.hh
        Array2Sqr2Iter.hh
        Array2Sqr2Iter2.hh
        #        Ravl2/Image/CornerDetectorSusan.cc Ravl2/Image/CornerDetectorSusan.hh
)

set_property(TARGET Ravl2Image PROPERTY CXX_STANDARD 20)
target_link_libraries(Ravl2Image PUBLIC Ravl2 Ravl2Geometry)

add_subdirectory(Segmentation)

add_executable(test_image tests/test_image.cc tests/testBoundary.cc)
target_link_libraries(test_image Ravl2Image GTest::gtest_main)
gtest_add_tests(TARGET test_image )

