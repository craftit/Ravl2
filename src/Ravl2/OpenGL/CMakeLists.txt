
include(GenerateExportHeader)
include(ExternalProject)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(glm CONFIG REQUIRED)


set(BUILD_IMGUIGLFW OFF CACHE BOOL "Build imgui with glfw")


# Get imgui

add_library (Ravl2OpenGL
  DObject.cc DObject.hh
  GLWindow.cc GLWindow.hh
  DPinholeCamera03D.cc DPinholeCamera03D.hh
  DCube3D.cc DCube3D.hh
  DLight3D.cc DLight3D.hh
  DPointSet3d.cc DPointSet3d.hh
  DTransform.cc DTransform.hh
  GLContext.cc GLContext.hh
  Canvas3D.cc Canvas3D.hh
  DTriMesh.cc DTriMesh.hh
  DTexTriMesh.cc DTexTriMesh.hh
  DViewPoint3D.cc DViewPoint3D.hh
  View3D.cc View3D.hh
  MouseEvent.cc MouseEvent.hh
  GLShader.cc  GLShader.hh
  DPolyLine.cc DPolyLine.hh
)

add_library(ravl2::Ravl2OpenGL ALIAS Ravl2OpenGL)

target_link_libraries(Ravl2OpenGL PRIVATE RAVL2_options RAVL2_warnings)
target_link_libraries(Ravl2OpenGL PUBLIC Ravl2x3D OpenGL::GLU OpenGL::GL GLUT::GLUT ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLU_LIBRARIES} ${GLFW_LIBRARIES} glm::glm)

target_include_directories(Ravl2OpenGL ${WARNING_GUARD} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${OpenCV_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${GLUT_INCLUDE_DIRS}
        ${GLU_INCLUDE_DIRS}
)

target_compile_features(Ravl2OpenGL PUBLIC cxx_std_23)

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

add_executable(test3d testCube.cc)
target_link_libraries (test3d PUBLIC Ravl2Image Ravl2OpenGL CLI11::CLI11)

if(BUILD_IMGUIGLFW)
    add_library (Ravl2ImGUI
            ImGui.cc ImGui.hh
    )
    target_link_libraries(Ravl2ImGUI PUBLIC imgui Ravl2OpenGL)
    target_link_libraries(viewer3d PUBLIC Ravl2ImGUI)
endif ()


