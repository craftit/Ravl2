cmake_minimum_required (VERSION 3.20)

project (Ravl2)

include(FetchContent)

FetchContent_Declare(
        xtensor
        GIT_REPOSITORY https://github.com/xtensor-stack/xtensor.git
        GIT_TAG        8c0a484f04eccd0dbc0e25eb58a97de000fb048b
        FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(xtensor)

FetchContent_Declare(
        xtensor-blas
        GIT_REPOSITORY https://github.com/xtensor-stack/xtensor-blas.git
        GIT_TAG        5dcbab5f41636a678a9ec715a058fbe6501aba77
        FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(xtensor-blas)

find_package(xtensor REQUIRED)
find_package(xtensor-blas REQUIRED)
find_package(GTest REQUIRED)
find_package(OpenCV REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
#find_package(lapack REQUIRED)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(Ravl2)
add_subdirectory(tests)
add_subdirectory(examples)

