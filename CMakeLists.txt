cmake_minimum_required(VERSION 3.21)

# This template attempts to be "fetch_content"-able
# so that it works well with tools like CPM or other
# manual dependency management

# Only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the project name and language
project(
  Ravl2
  VERSION 0.0.1
  DESCRIPTION "Recognition and Vision Library 2"
  HOMEPAGE_URL "https://github.com/craftit/Ravl2"
  LANGUAGES CXX C)

include(cmake/PreventInSourceBuilds.cmake)
include(cmake/LoggingConfig.cmake)

# print cmake version
message(STATUS "CMake version: ${CMAKE_VERSION}")

include(ProjectOptions.cmake)

RAVL2_setup_options()
# Configure logging levels based on build type
configure_logging_level()


RAVL2_global_options()
# Don't use, or write to, the user-wide CMake package registry (~/.cmake/packages). Eigen's CMake registers its
# build directory there by default, which leaves stale entries that other projects (and later configures of this
# one) then find instead of the intended package.
set(CMAKE_EXPORT_PACKAGE_REGISTRY OFF)
set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON)

include(Dependencies.cmake)
RAVL2_setup_dependencies()

RAVL2_local_options()

# don't know if this should be set globally from here or not...
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(GIT_SHA
    "Unknown"
    CACHE STRING "SHA this build was generated from")
string(
  SUBSTRING "${GIT_SHA}"
            0
            8
            GIT_SHORT_SHA)

target_compile_features(RAVL2_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

add_library(ravl2::RAVL2_options ALIAS RAVL2_options)
add_library(ravl2::RAVL2_warnings ALIAS RAVL2_warnings)

#add_library(ravl2::RAVL2_options INTERFACE IMPORTED)
#add_library(ravl2::RAVL2_warnings INTERFACE IMPORTED)

# configure files based on CMake configuration options
add_subdirectory(configured_files)

# Adding the src:
add_subdirectory(src)

# Don't even look at tests if we're not top level
if(NOT PROJECT_IS_TOP_LEVEL)
  return()
endif()

if(RAVL2_BUILD_EXAMPLES)
#  message(AUTHOR_WARNING "Building Examples")
  add_subdirectory(examples)
endif()

# Adding the tests:
include(CTest)

if(BUILD_TESTING)
  #message(AUTHOR_WARNING "Building Tests. Be sure to check out test/constexpr_tests.cpp for constexpr testing")
  add_subdirectory(test)
endif()


if(RAVL2_BUILD_FUZZ_TESTS)
  message(AUTHOR_WARNING "Building Fuzz Tests, using fuzzing sanitizer https://www.llvm.org/docs/LibFuzzer.html")
  if (NOT RAVL2_ENABLE_ADDRESS_SANITIZER AND NOT RAVL2_ENABLE_THREAD_SANITIZER)
    message(WARNING "You need asan or tsan enabled for meaningful fuzz testing")
  endif()
  add_subdirectory(fuzz_test)

endif()

# If MSVC is being used, and ASAN is enabled, we need to set the debugger environment
# so that it behaves well with MSVC's debugger, and we can run the target from visual studio
if(MSVC)
  get_all_installable_targets(all_targets)
  message("all_targets=${all_targets}")
  set_target_properties(${all_targets} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=$(VC_ExecutablePath_x64);%PATH%")
endif()

# set the startup project for the "play" button in MSVC
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT intro)

if(CMAKE_SKIP_INSTALL_RULES)
  return()
endif()

include(cmake/PackageProject.cmake)
include(GNUInstallDirs)

# Installed core: `cmake --install` then `find_package(ravl2 CONFIG)` and link ravl2::<target>.
# Display, OpenGL, Qt and Dlib aren't installed (their dependencies are built in-tree); use them from the build tree.
set(RAVL2_INSTALL_TARGETS)
foreach(_t Ravl2Core Ravl2Math Ravl2Geometry Ravl2Image Ravl2ImageSegmentation Ravl2ImageIO Ravl2x3D Ravl2Optimise
        Ravl2Video Ravl2GoPro Ravl2OpenCV)
  if(TARGET ${_t})
    list(APPEND RAVL2_INSTALL_TARGETS ${_t})
    # Build-tree include paths are BUILD_INTERFACE only; installed consumers find headers here.
    target_include_directories(${_t} PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  endif()
endforeach()
# The GPMF parser is built in-tree and linked privately by Ravl2GoPro, so a static install has to carry it.
if(TARGET GPMF_PARSER_LIB AND TARGET Ravl2GoPro)
  list(APPEND RAVL2_INSTALL_TARGETS GPMF_PARSER_LIB)
endif()

install(DIRECTORY src/Ravl2/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Ravl2
        FILES_MATCHING PATTERN "*.hh"
        PATTERN "Display" EXCLUDE PATTERN "OpenGL" EXCLUDE PATTERN "Qt" EXCLUDE PATTERN "Dlib" EXCLUDE
        PATTERN "ThirdParty" EXCLUDE)
# Generated headers: export macros and the configured Ravl2/config.hh.
install(DIRECTORY ${PROJECT_BINARY_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(RAVL2_PUBLIC_DEPENDENCIES "fmt" "spdlog" "Eigen3" "cereal" "nlohmann_json")
if(TARGET Ravl2OpenCV)
  list(APPEND RAVL2_PUBLIC_DEPENDENCIES "OpenCV")
endif()

RAVL2_package_project(
  # Lower case, to match the ravl2:: aliases used in the build tree.
  NAME ravl2
  TARGETS
  ${RAVL2_INSTALL_TARGETS}
  RAVL2_options
  RAVL2_warnings
  PUBLIC_DEPENDENCIES ${RAVL2_PUBLIC_DEPENDENCIES}
  # Linked privately; only needed by consumers of static libraries.
  PRIVATE_DEPENDENCIES "JPEG" "PNG"
)

# Experience shows that explicit package naming can help make it easier to sort
# out potential ABI related issues before they start, while helping you
# track a build to a specific GIT SHA
set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(CPack)
