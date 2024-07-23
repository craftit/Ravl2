include(cmake/CPM.cmake)

# Old way of doing things
include(FetchContent)

FetchContent_Declare(
        xtensor
        GIT_REPOSITORY https://github.com/xtensor-stack/xtensor.git
        GIT_TAG        8c0a484f04eccd0dbc0e25eb58a97de000fb048b
        FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(xtensor)

FetchContent_Declare(
        xtensor-blas
        GIT_REPOSITORY https://github.com/xtensor-stack/xtensor-blas.git
        GIT_TAG        5dcbab5f41636a678a9ec715a058fbe6501aba77
        FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(xtensor-blas)


####################################################################################################

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(ravl2_setup_dependencies)

  # For each dependency, see if it's
  # already been provided to us by a parent project

  if(NOT TARGET fmtlib::fmtlib)
    cpmaddpackage("gh:fmtlib/fmt#9.1.0")
  endif()

  if(NOT TARGET spdlog::spdlog)
    cpmaddpackage(
      NAME
      spdlog
      VERSION
      1.11.0
      GITHUB_REPOSITORY
      "gabime/spdlog"
      OPTIONS
      "SPDLOG_FMT_EXTERNAL ON")
  endif()

  if(NOT TARGET Catch2::Catch2WithMain)
    cpmaddpackage("gh:catchorg/Catch2@3.3.2")
  endif()

  if(NOT TARGET CLI11::CLI11)
    cpmaddpackage("gh:CLIUtils/CLI11@2.3.2")
  endif()

#  if(NOT TARGET xtl)
#    cpmaddpackage("gh:xtensor-stack/xtl@0.7.7")
#  endif()
#
#  if(NOT TARGET xtensor)
#    cpmaddpackage("gh:xtensor-stack/xtensor@0.23.10")
#  endif()

#  if(NOT TARGET xtensor-blas)
#    cpmaddpackage("gh/xtensor-stack/xtensor-blas@0.17.1")
#  endif()

#  if(NOT TARGET OpenCV::OpenCV)
#    cpmaddpackage("gh:opencv/opencv@4.5.3")
#  endif()

  #CPMAddPackage("gh:nlohmann/json@3.10.5")

  if(NOT TARGET tools::tools)
    cpmaddpackage("gh:lefticus/tools#update_build_system")
  endif()

endfunction()
