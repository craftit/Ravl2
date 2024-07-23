

add_library(Ravl2ImageSegmentation STATIC
        Crack.cc Crack.hh
        CrackCode.cc CrackCode.hh
        Boundary.cc Boundary.hh
        Segmentation.hh Segmentation.cc
        Boundary.tcc
)

set_property(TARGET Ravl2ImageSegmentation PROPERTY CXX_STANDARD 20)

target_link_libraries(Ravl2ImageSegmentation PUBLIC Ravl2Image)

add_executable(test_segmentation  tests/testBoundary.cc)
target_link_libraries(test_segmentation Ravl2ImageSegmentation GTest::gtest_main)
gtest_add_tests(TARGET test_segmentation )
