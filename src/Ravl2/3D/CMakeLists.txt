
include(GenerateExportHeader)

add_library (Ravl2x3D
  PinholeCamera.cc PinholeCamera.hh
  PinholeCamera0.cc PinholeCamera0.hh
  PinholeCamera1.cc PinholeCamera1.hh
  PinholeCamera2.cc PinholeCamera2.hh
  PinholeCamera3.cc PinholeCamera3.hh
  PinholeCameraArray.cc PinholeCameraArray.hh
  Vertex.cc Vertex.hh
  Tri.cc Tri.hh
  TriMesh.cc TriMesh.hh
  TexTriMesh.cc TexTriMesh.hh
  MeshShapes.cc MeshShapes.hh
)

add_library(ravl2::Ravl2x3D ALIAS Ravl2x3D)

target_link_libraries(Ravl2x3D PRIVATE RAVL2_options RAVL2_warnings)
target_link_libraries(Ravl2x3D PUBLIC Ravl2Core Ravl2Geometry)

target_include_directories(Ravl2x3D ${WARNING_GUARD} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

target_compile_features(Ravl2x3D PUBLIC cxx_std_20)

set_target_properties(
        Ravl2x3D
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(Ravl2x3D EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/ravl2/Ravl2Core_export.hpp)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(Ravl2x3D PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()


