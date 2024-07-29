include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(RAVL2_setup_dependencies)

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

  if(NOT TARGET xtl)
    cpmaddpackage(
            NAME
            xtl
            GITHUB_REPOSITORY
            "xtensor-stack/xtl"
            VERSION
            0.7.7
            GIT_TAG
            "0.7.7"
    )
  endif()

  if(NOT TARGET xtensor)
    cpmaddpackage(
            NAME
            xtensor
            GITHUB_REPOSITORY
            "xtensor-stack/xtensor"
            VERSION
            0.23.10
            GIT_TAG
            "0.23.10"
            )
  endif()

  if(NOT TARGET xtensor-blas)
    cpmaddpackage(
            NAME
            xtensor-blas
            GITHUB_REPOSITORY
            "xtensor-stack/xtensor-blas"
            VERSION
            0.17.1
            GIT_TAG
            "0.17.1"
            )
  endif()

#  if(NOT TARGET OpenCV::OpenCV)
#    cpmaddpackage("gh:opencv/opencv@4.5.3")
#  endif()

  #CPMAddPackage("gh:nlohmann/json@3.10.5")

  if(NOT TARGET tools::tools)
    cpmaddpackage("gh:lefticus/tools#update_build_system")
  endif()

endfunction()
