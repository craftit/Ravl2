
include(GenerateExportHeader)
include(ExternalProject)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
find_package(OpenGL REQUIRED)

set(BUILD_IMGUIGLFW OFF CACHE BOOL "Build imgui with glfw")

if(BUILD_IMGUIGLFW)
    FetchContent_Declare(imgui_external
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(imgui_external)

    add_library(imgui
            ${imgui_external_SOURCE_DIR}/imgui.cpp
            ${imgui_external_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_external_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_external_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_external_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_external_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
            ${imgui_external_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )

    # Set compile flags for imgui.  Suppress useless cast
    # If we're using gcc
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(imgui PRIVATE -Wno-old-style-cast -Wno-useless-cast)
    else ()
        target_compile_options(imgui PRIVATE -Wno-old-style-cast)
    endif()

    target_include_directories(imgui PUBLIC ${imgui_external_SOURCE_DIR})
    target_link_libraries(imgui PUBLIC ${GLFW_LIBRARIES} )
endif ()


# Get imgui

add_library (Ravl2OpenGL
  DObject3D.cc DObject3D.hh
  GLWindow.cc GLWindow.hh
  DPinholeCamera03D.cc DPinholeCamera03D.hh
  DCube3D.cc DCube3D.hh
  DLight3D.cc DLight3D.hh
  DPointSet3d.cc DPointSet3d.hh
  DTransform.cc DTransform.hh
  GLContext.cc GLContext.hh
  Canvas3D.cc Canvas3D.hh
  DTriMesh3D.cc DTriMesh3D.hh
  DTexTriMesh3D.cc DTexTriMesh3D.hh
  DViewPoint3D.cc DViewPoint3D.hh
)

add_library(ravl2::Ravl2OpenGL ALIAS Ravl2OpenGL)

target_link_libraries(Ravl2OpenGL PRIVATE RAVL2_options RAVL2_warnings)
target_link_libraries(Ravl2OpenGL PUBLIC Ravl2x3D ${OPENGL_LIBRARIES} ${GLFW_LIBRARIES})

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

if(BUILD_IMGUIGLFW)
target_link_libraries(viewer3d PRIVATE imgui)
add_library (Ravl2ImGUI
        ImGui.cc ImGui.hh
)
endif ()


