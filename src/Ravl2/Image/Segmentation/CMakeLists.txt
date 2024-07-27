
include(GenerateExportHeader)

add_library (Ravl2ImageSegmentation
        Crack.cc Crack.hh
        CrackCode.cc CrackCode.hh
        Boundary.cc Boundary.hh
        Segmentation.hh Segmentation.cc
        FloodRegion.cc FloodRegion.hh
        SegmentExtrema.cc SegmentExtrema.hh
)

add_library(ravl2::Ravl2ImageSegmentation ALIAS Ravl2ImageSegmentation)

target_link_libraries(Ravl2ImageSegmentation PRIVATE ravl2_options ravl2_warnings
)

target_link_libraries(Ravl2ImageSegmentation PUBLIC Ravl2Core Ravl2Image)

target_include_directories(Ravl2ImageSegmentation ${WARNING_GUARD} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

target_compile_features(Ravl2ImageSegmentation PUBLIC cxx_std_20)

set_target_properties(
        Ravl2ImageSegmentation
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(Ravl2ImageSegmentation EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/ravl2/Ravl2Core_export.hpp)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(Ravl2ImageSegmentation PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()


