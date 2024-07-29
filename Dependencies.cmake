include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(RAVL2_setup_dependencies)

  # Try and use native packages if they're available

  find_package(fmt QUIET)
  find_package(spdlog QUIET)
  find_package(xtensor QUIET)
  find_package(xtensor-blas QUIET)
  find_package(Catch2 3 QUIET)
  find_package(CLI11 QUIET)
  find_package(nlohmann_json QUIET)

  # Sort out some blas
  find_package(BLAS REQUIRED)
  find_package(LAPACK REQUIRED)

  # For each dependency, see if it's
  # already been provided to us by a parent project

  if(NOT TARGET fmt::fmt)
    cpmaddpackage("gh:fmtlib/fmt#9.1.0")
  else()
    message(STATUS "Found native fmt::fmt")
    add_library(libfmt::libfmt ALIAS fmt::fmt)
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
  else ()
    message(STATUS "Found native spdlog::spdlog")
  endif()

  if(NOT TARGET Catch2::Catch2WithMain)
    cpmaddpackage("gh:catchorg/Catch2@3.3.2")
  else()
    message(STATUS "Found native Catch2::Catch2WithMain")
  endif()

  if(NOT TARGET CLI11::CLI11)
    cpmaddpackage("gh:CLIUtils/CLI11@2.3.2")
  else()
    message(STATUS "Found native CLI11::CLI11")
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
  else()
    message(STATUS "Found native xtl")
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
  else()
    message(STATUS "Found native xtensor")
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
  else ()
    message(STATUS "Found native xtensor-blas")
  endif()

  if(NOT TARGET nlohmann_json::nlohmann_json)
    CPMAddPackage("gh:nlohmann/json@3.10.5")
  else()
    message(STATUS "Found native nlohmann_json::nlohmann_json")
  endif()

#  if(NOT TARGET tools::tools)
#    cpmaddpackage("gh:lefticus/tools#update_build_system")
#  endif()

endfunction()
