

add_library (Ravl2Geometry
        Geometry.hh Geometry.cc
        Affine.cc Affine.hh
        Affine2f.cc Affine2f.hh
        Affine3d.cc Affine3d.hh
        PointSet.cc PointSet.hh
        Moments2.cc Moments2.hh
        CircleIter.cc CircleIter.hh
        Circle.cc Circle.hh
        FLinePP.hh FLinePP.cc
        LinePP2d.hh LinePP2d.cc
        Line2dIter.hh Line2dIter.cc
        Polygon2d.cc Polygon2d.hh
        Polygon2dIter.cc Polygon2dIter.hh
        Range.hh Range.cc
)

target_link_libraries(Ravl2Geometry PUBLIC Ravl2 )


add_executable(test_geometry tests/test_geometry.cc)
target_link_libraries (test_geometry Ravl2Geometry GTest::gtest_main xtensor xtensor-blas )
gtest_add_tests(TARGET test_geometry )
