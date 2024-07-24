
include(GenerateExportHeader)

add_library (Ravl2OpenCV
        convert.hh convert.cc
)

add_library(ravl2::Ravl2OpenCV ALIAS Ravl2OpenCV)

target_link_libraries(Ravl2OpenCV PRIVATE ravl2_options ravl2_warnings)
target_link_libraries(Ravl2OpenCV PUBLIC Ravl2Image  ${OpenCV_LIBS})

target_include_directories(Ravl2OpenCV ${WARNING_GUARD} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${OpenCV_INCLUDE_DIRS}
)

target_compile_features(Ravl2OpenCV PUBLIC cxx_std_20)

set_target_properties(
        Ravl2OpenCV
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(Ravl2OpenCV EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/ravl2/Ravl2Core_export.hpp)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(Ravl2OpenCV PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()


