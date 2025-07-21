

include(GenerateExportHeader)

add_library (Ravl2Optimise
  Optimise.cc Optimise.hh
  OptimiseBrent.hh OptimiseBrent.cc
  BracketMinimum.cc BracketMinimum.hh
  OptimisePowell.cc OptimisePowell.hh
  GaussianProcess.cc  GaussianProcess.hh
  BayesianOptimise.cc BayesianOptimise.hh
)

add_library(ravl2::Ravl2Optimise ALIAS Ravl2Optimise)

target_link_libraries(Ravl2Optimise PRIVATE RAVL2_options RAVL2_warnings)
target_link_libraries(Ravl2Optimise PUBLIC Ravl2Core )

target_include_directories(Ravl2Optimise ${WARNING_GUARD} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
)

target_compile_features(Ravl2Optimise PUBLIC cxx_std_20)

set_target_properties(
        Ravl2Optimise
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(Ravl2Optimise EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/ravl2/Ravl2Core_export.hpp)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(Ravl2Optimise PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()


