


add_executable(test_geometry tests/test_geometry.cc)
target_link_libraries (test_geometry Ravl2 GTest::gtest_main xtensor xtensor-blas )
gtest_add_tests(TARGET test_geometry )
