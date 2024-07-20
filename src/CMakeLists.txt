cmake_minimum_required (VERSION 3.20)

project (Ravl2)

find_package(xtensor REQUIRED)
find_package(xtensor-blas REQUIRED)
find_package(GTest REQUIRED)
find_package(OpenCV REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
include(GoogleTest)

set(CMAKE_CXX_STANDARD 20)

add_library (Ravl2
        Ravl2/Index.cc Ravl2/Index.hh
        Ravl2/Array.hh
        Ravl2/ArrayIterZip.hh
        Ravl2/ScanWindow.hh
        Ravl2/Geometry/Geometry.hh Ravl2/Geometry/Geometry.cc
        Ravl2/Geometry/Affine.hh
        Ravl2/Geometry/Affine2f.cc Ravl2/Geometry/Affine2f.hh
        Ravl2/Geometry/Affine3d.cc Ravl2/Geometry/Affine3d.hh
        Ravl2/Geometry/Moments2.cc Ravl2/Geometry/Moments2.hh
)


set_property(TARGET Ravl2 PROPERTY CXX_STANDARD 20)
target_link_libraries(Ravl2 PUBLIC ${OpenCV_LIBS} fmt::fmt xtensor xtensor-blas spdlog::spdlog )

message(STATUS "Current dir: ${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(Ravl2 PUBLIC ".")

add_subdirectory(Ravl2/Image)
add_subdirectory(Ravl2/Geometry)

add_executable(benchmarkAccess Ravl2/benchmarkAccess.cc )
target_link_libraries (benchmarkAccess PUBLIC Ravl2 )

add_executable(test_core test/test_core.cc)
target_link_libraries (test_core Ravl2 GTest::gtest_main)
gtest_add_tests(TARGET test_core )

