
include(GenerateExportHeader)

find_package(Qt6 COMPONENTS Core Gui REQUIRED)

add_library (Ravl2Qt
    Image.hh Image.cc
)

add_library(ravl2::Ravl2Qt ALIAS Ravl2Qt)

target_link_libraries(Ravl2Qt PRIVATE RAVL2_options RAVL2_warnings)
target_link_libraries(Ravl2Qt PUBLIC Ravl2Image Qt6::Core Qt6::Gui)

target_include_directories(Ravl2Qt ${WARNING_GUARD} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${OpenCV_INCLUDE_DIRS}
)

target_compile_features(Ravl2Qt PUBLIC cxx_std_20)

set_target_properties(
        Ravl2Qt
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(Ravl2Qt EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/ravl2/Ravl2Core_export.hpp)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(Ravl2Qt PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()


