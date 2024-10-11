
include(GenerateExportHeader)

find_package(OpenGL REQUIRED)

add_library (Ravl2OpenGL
  DObject3D.cc DObject3D.hh
  GLWindow.cc GLWindow.hh
  DPinholeCamera03D.cc DPinholeCamera03D.hh
  DCube3D.cc DCube3D.hh
  DLight3D.cc DLight3D.hh
  DPointSet3d.cc DPointSet3d.hh
  Util.cc Util.hh
)

add_library(ravl2::Ravl2OpenGL ALIAS Ravl2OpenGL)

target_link_libraries(Ravl2OpenGL PRIVATE RAVL2_options RAVL2_warnings)
target_link_libraries(Ravl2OpenGL PUBLIC Ravl2x3D ${OPENGL_LIBRARIES} glfw )

target_include_directories(Ravl2OpenGL ${WARNING_GUARD} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${OpenCV_INCLUDE_DIRS}
)

target_compile_features(Ravl2OpenGL PUBLIC cxx_std_20)

set_target_properties(
        Ravl2OpenGL
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(Ravl2OpenGL EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/ravl2/Ravl2Core_export.hpp)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(Ravl2OpenGL PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()


add_executable(viewer3d viewer3d.cc)
target_link_libraries (viewer3d PUBLIC Ravl2Image Ravl2OpenGL CLI11::CLI11)

