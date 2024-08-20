
find_package(xtensor REQUIRED)
find_package(xtensor-blas REQUIRED)
find_package(OpenCV REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(cereal REQUIRED)


find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)
find_package(dlib QUIET)
find_package(Qt5 QUIET)

add_subdirectory(Geometry)
add_subdirectory(Math)
add_subdirectory(Pixel)
add_subdirectory(Image)
add_subdirectory(OpenCV)

# If dlib is found, add the dlib subdirectory
if(TARGET dlib::dlib)
    add_subdirectory(Dlib)
endif()

if(TARGET Qt${QT_VERSION_MAJOR}::Core)
    add_subdirectory(Qt)
endif()

include(GenerateExportHeader)

add_library(Ravl2Core
    Index.cc Index.hh
    Angle.hh Angle.cc
    Array1.cc Array1.hh
    Array.cc Array.hh
    ArrayIterZip.cc ArrayIterZip.hh
    ScanWindow.cc ScanWindow.hh
    Assert.hh
    IndexRangeSet.cc IndexRangeSet.hh
    Sentinel.cc Sentinel.hh
    Types.cc Types.hh
    Resource.cc Resource.hh
    LoopIter.cc LoopIter.hh
    IndexRange1.cc IndexRange1.hh
    IndexRange.cc IndexRange.hh
    Concepts.hh
    CallbackArray.cc CallbackArray.hh
    FileIO.cc FileIO.hh
    StringUtils.cc StringUtils.hh
    Base64.cc Base64.hh
    Configuration.cc Configuration.hh
    ConfigJson.cc ConfigJson.hh
        SequenceIO.cc
        SequenceIO.hh
)

add_library(ravl2::Ravl2Core ALIAS Ravl2Core)

target_link_libraries(Ravl2Core PRIVATE RAVL2_options RAVL2_warnings)

# On apple we need to add lapack and blas
if (APPLE)
    target_link_libraries(Ravl2Core PUBLIC "-framework Accelerate")
endif()


target_link_libraries(Ravl2Core PUBLIC ${OpenCV_LIBS} cereal::cereal fmt::fmt xtensor xtensor-blas spdlog::spdlog BLAS::BLAS LAPACK::LAPACK)

target_include_directories(Ravl2Core ${WARNING_GUARD} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

message(STATUS "RAVL2 Binary     : ${PROJECT_BINARY_DIR}")
message(STATUS "RAVL2 Installing : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "RAVL2 Source     : ${PROJECT_SOURCE_DIR}")

target_compile_definitions(Ravl2Core PRIVATE -DRAVL_SOURCE_DIR="${PROJECT_SOURCE_DIR}" -DRAVL_BINARY_DIR="${PROJECT_BINARY_DIR}" -DRAVL_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")


target_compile_features(Ravl2Core PUBLIC cxx_std_20)

set_target_properties(
        Ravl2Core
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(Ravl2Core EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/ravl2/Ravl2Core_export.hpp)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(Ravl2Core PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()
