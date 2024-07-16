cmake_minimum_required (VERSION 3.20)

project (Ravl2)

find_package(xtensor REQUIRED)
find_package(GTest REQUIRED)
include(GoogleTest)

set(CMAKE_CXX_STANDARD 20)

add_library (Ravl2
        Ravl2/Index.cc Ravl2/Index.hh
        Ravl2/Image/EdgeSobel.cc Ravl2/Image/EdgeSobel.hh
        Ravl2/Image/BilinearInterpolation.hh
)

message(STATUS "Current dir: ${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(Ravl2 PUBLIC ".")

add_executable(benchmarkAccess Ravl2/benchmarkAccess.cc)
target_link_libraries (benchmarkAccess PUBLIC xtensor Ravl2)

add_executable(test_core test/test_core.cc)
target_link_libraries (test_core Ravl2  GTest::gtest GTest::gtest_main)
gtest_add_tests(TARGET test_core )

add_executable(test_image test/test_image.cc)
target_link_libraries(test_image Ravl2  GTest::gtest GTest::gtest_main)
gtest_add_tests(TARGET test_image )