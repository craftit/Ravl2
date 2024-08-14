
include(GenerateExportHeader)

add_library (Ravl2Math
        Sums1d2.cc Sums1d2.hh
        MeanVariance.cc MeanVariance.hh
        FastFourierTransform.cc FastFourierTransform.hh
        PrimeFactors.cc  PrimeFactors.hh
        FastFourierTransformMath.hh
        LinearAlgebra.cc LinearAlgebra.hh
)

add_library(ravl2::Ravl2Math ALIAS Ravl2Math)

target_link_libraries(Ravl2Math PRIVATE RAVL2_options RAVL2_warnings)
target_link_libraries(Ravl2Math PUBLIC Ravl2Core )

target_include_directories(Ravl2Math ${WARNING_GUARD} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
)

target_compile_features(Ravl2Math PUBLIC cxx_std_20)

set_target_properties(
        Ravl2Math
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(Ravl2Math EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/ravl2/Ravl2Core_export.hpp)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(Ravl2Math PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()


