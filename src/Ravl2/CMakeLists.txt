
#add_library (Ravl2
#        Index.cc Index.hh
#        Array.hh Array.cc
#        ArrayIterZip.hh
#        ScanWindow.hh
#        Assert.hh
#)

#set_property(TARGET Ravl2 PROPERTY CXX_STANDARD 20)
#target_link_libraries(Ravl2 PUBLIC ${OpenCV_LIBS} fmt::fmt xtensor xtensor-blas spdlog::spdlog lapack )
#target_include_directories(Ravl2 PUBLIC "..")

add_subdirectory(Image)
add_subdirectory(Geometry)

include(GenerateExportHeader)

add_library(Ravl2Core
    Index.cc Index.hh
    Array.hh Array.cc
    ArrayIterZip.hh
    ScanWindow.hh
    Assert.hh
)

add_library(ravl2::Ravl2Core ALIAS Ravl2Core)

target_link_libraries(Ravl2Core PRIVATE ravl2_options ravl2_warnings
)

target_link_libraries(Ravl2Core PUBLIC ${OpenCV_LIBS} fmt::fmt xtensor xtensor-blas spdlog::spdlog lapack)

target_include_directories(Ravl2Core ${WARNING_GUARD} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

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
