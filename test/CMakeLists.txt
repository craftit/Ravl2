cmake_minimum_required(VERSION 3.15...3.23)

project(CmakeConfigPackageTests LANGUAGES CXX)

# ---- Test as standalone project the exported config package ----

if(PROJECT_IS_TOP_LEVEL OR TEST_INSTALLED_VERSION)
  enable_testing()

  find_package(ravl2 CONFIG REQUIRED) # for intro, project_options, ...

  if(NOT TARGET RAVL2_options)
    message(FATAL_ERROR "Required config package not found!")
    return() # be strictly paranoid for Template Janitor github action! CK
  endif()
endif()

# ---- Dependencies ----
# The following prevents the tests from being deleted if test discovery fails
#set(CMAKE_CATCH_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

find_package(Catch2 3 REQUIRED)

# If we're using fetchContent, we need to add the Catch2 extra directory to the module path
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
# The following is needed on ubuntu 24.04
list(APPEND CMAKE_MODULE_PATH ${Catch2_DIR})

include(CTest)
include(Catch)

if(PROJECT_IS_TOP_LEVEL OR TEST_INSTALLED_VERSION)
  # Provide a simple smoke test to make sure that the CLI works and can display a --help message
  add_test(NAME cli.has_help COMMAND intro --help)

  # Provide a test to verify that the version being reported from the application
  # matches the version given to CMake. This will be important once you package
  # your program. Real world shows that this is the kind of simple mistake that is easy
  # to make, but also easy to test for.
  add_test(NAME cli.version_matches COMMAND intro --version)
  set_tests_properties(cli.version_matches PROPERTIES PASS_REGULAR_EXPRESSION "${PROJECT_VERSION}")
endif ()

add_executable(test_call test_calls.cc)
target_link_libraries(test_call PRIVATE ravl2::RAVL2_warnings ravl2::RAVL2_options ravl2::Ravl2Core Catch2::Catch2WithMain)


add_executable(tests
        test_index.cc
        test_core.cc
        test_math.cc
        test_geometry.cc
        test_polygon.cc
        test_image.cc
        test_harrisAffine.cc
        test_draw.cc
        test_boundary.cc
        test_segmentation.cc
        test_pixel.cc
        test_lines.cc
        test_edgeDetector.cc
        test_CallbackArray.cc
        test_base64.cc
        test_configuration.cc
        test_quaternion.cc
        test_RankedScoreArray.cc
        test_3d.cc
        test_io.cc
        test_ThreadedQueue.cc
        test_warpScale.cc
        test_optimise.cc
        test_warpScale.cc
        test_projective.cc
        test_eigen.cc
        test_planes.cc
        test_pixelplane.cc
        test_PlanarImage.cc
        test_spatialhash.cc
        test_BayesianOptimise.cc
        test_RandomSearchOptimise.cc
        test_ParticleSwarmOptimise.cc
        test_OptimiseBobyqa.cc
        test_MemoryMediaContainer.cc
        test_VideoFrame.cc
        test_VideoIO.cc
        test_VideoWriter.cc
        test_imageio_jpeg.cc
        test_imageio_jpeg_save.cc
        test_imageio_jpeg_planar.cc
        test_imageio_png.cc
        test_types_duration.cc
        test_types.cc
        test_array_ranges.cc
        test_gps.cc
        test_instantiation.cc
        test_capture.cc
)
target_compile_definitions(tests PRIVATE -DRAVL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_libraries(
  tests
  PRIVATE ravl2::RAVL2_warnings
          ravl2::RAVL2_options
          ravl2::Ravl2Core
          ravl2::Ravl2Geometry
          ravl2::Ravl2Math
          ravl2::Ravl2Image
          ravl2::Ravl2ImageSegmentation
          ravl2::Ravl2x3D
          ravl2::Ravl2ImageIO
          ravl2::Ravl2Video
          ravl2::Ravl2Optimise
          Catch2::Catch2WithMain
          Eigen3::Eigen)
# The Display stack is optional (RAVL2_ENABLE_DISPLAY_STACK).
if(TARGET ravl2::Ravl2Display)
  target_sources(tests PRIVATE Display_Normalization_tests.cc Display_3D_E2E_tests.cc Display_Commands_tests.cc)
  target_link_libraries(tests PRIVATE ravl2::Ravl2Display)
endif()
# Only test_segmentation's DODEBUG path uses the OpenCV bridge.
if(TARGET ravl2::Ravl2Qt)
  target_sources(tests PRIVATE test_qt.cc)
  target_link_libraries(tests PRIVATE ravl2::Ravl2Qt)
endif()
if(TARGET ravl2::Ravl2OpenCV)
  target_sources(tests PRIVATE test_opencv.cc)
  target_link_libraries(tests PRIVATE ravl2::Ravl2OpenCV)
endif()

# The GoPro module is optional -- GoPro/CMakeLists.txt returns early without a GPMF parser, so the
# target may not exist. Add its tests only when it does.
if(TARGET Ravl2GoPro)
  target_sources(tests PRIVATE test_videoGpsSampler.cc)
  target_link_libraries(tests PRIVATE ravl2::Ravl2GoPro)
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(
    TARGET tests
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:tests> $<TARGET_FILE_DIR:tests>
    COMMAND_EXPAND_LISTS)
endif()

# automatically discover tests that are defined in catch based test files you can modify the unittests. Set TEST_PREFIX
# to whatever you want, or use different for different binaries
catch_discover_tests(
  tests
  TEST_PREFIX
  "ravl2_unittests."
  REPORTER
  XML
  OUTPUT_DIR
  .
  OUTPUT_PREFIX
  "ravl2_unittests."
  OUTPUT_SUFFIX
  .xml
  # Catch2 exits with 4 when everything it ran was skipped (e.g. no camera for the capture test): report that
  # as skipped, not failed.
  PROPERTIES
  SKIP_RETURN_CODE
  4)

# Add a file containing a set of constexpr tests
add_executable(constexpr_tests constexpr_tests.cpp)
target_link_libraries(
  constexpr_tests
  PRIVATE ravl2::RAVL2_warnings
          ravl2::RAVL2_options
          ravl2::Ravl2Core
          Catch2::Catch2WithMain)

catch_discover_tests(
  constexpr_tests
  TEST_PREFIX
  "ravl2_constexpr."
  REPORTER
  XML
  OUTPUT_DIR
  .
  OUTPUT_PREFIX
  "ravl2_constexpr."
  OUTPUT_SUFFIX
  .xml)

# Disable the constexpr portion of the test, and build again this allows us to have an executable that we can debug when
# things go wrong with the constexpr testing
add_executable(relaxed_constexpr_tests constexpr_tests.cpp)
target_link_libraries(
  relaxed_constexpr_tests
  PRIVATE ravl2::RAVL2_warnings
          ravl2::RAVL2_options
          ravl2::Ravl2Core
          Catch2::Catch2WithMain)
target_compile_definitions(relaxed_constexpr_tests PRIVATE -DCATCH_CONFIG_RUNTIME_STATIC_REQUIRE)

catch_discover_tests(
  relaxed_constexpr_tests
  TEST_PREFIX
  "ravl2_relaxed_constexpr."
  REPORTER
  XML
  OUTPUT_DIR
  .
  OUTPUT_PREFIX
  "ravl2_relaxed_constexpr."
  OUTPUT_SUFFIX
  .xml)
