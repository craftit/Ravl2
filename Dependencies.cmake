include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(RAVL2_setup_dependencies)

  # Sort out some blas/lapack stuff
  #set(BLAS_LIBS /usr/lib/x86_64-linux-gnu/blas)
  #set(BLA_VENDOR "Generic")
  #set(BLA_PKGCONFIG_BLAS "blas")

  #if on apple and BLA_VENDOR is not set, set it to Apple
  if (APPLE AND NOT DEFINED BLA_VENDOR)
    if(APPLE)
      set(BLA_VENDOR "Apple")
    else()
      set(BLA_VENDOR "All")
    endif ()
  else ()
    set(BLA_VENDOR "All")
  endif()

  find_package(BLAS REQUIRED)
  find_package(LAPACK REQUIRED)

  # Try and use native packages if they're available

  find_package(fmt QUIET)
  find_package(spdlog QUIET)
  find_package(Eigen3 3.4 QUIET)
  find_package(Catch2 3 QUIET)
  find_package(CLI11 QUIET)
  find_package(nlohmann_json QUIET)
  find_package(cereal QUIET)
  find_package(OpenGL QUIET)

  # Optional dependencies, we won't build them if they're not found

  find_package(dlib QUIET)
  find_package(glfw3 QUIET)

  find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS  Core Gui Widgets)


  # For each dependency, see if it's
  # already been provided to us by a parent project

  if(NOT TARGET fmt::fmt)
    cpmaddpackage("gh:fmtlib/fmt#9.1.0")
  else()
    message(STATUS "Found native fmt::fmt")
#    if (NOT TARGET libfmt::libfmt)
#      add_library(libfmt::libfmt ALIAS fmt::fmt)
#    endif ()
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
    message(STATUS "Found native Catch2::Catch2")
  endif()

  if(NOT TARGET CLI11::CLI11)
    cpmaddpackage("gh:CLIUtils/CLI11@2.3.2")
  else()
    message(STATUS "Found native CLI11::CLI11")
  endif()

  if(NOT TARGET Eigen3::Eigen)
    cpmaddpackage(
            NAME
            eigen
            GITLAB_REPOSITORY
            "libeigen/eigen"
            VERSION
            3.4.0
            GIT_TAG
            "3.4.0"
    )
  else()
    message(STATUS "Found native Eigen3::Eigen")
  endif()

  if(NOT TARGET nlohmann_json::nlohmann_json)
    CPMAddPackage("gh:nlohmann/json@3.10.5")
  else()
    message(STATUS "Found native nlohmann_json::nlohmann_json")
  endif()

  if(NOT TARGET cereal::cereal)
    cpmaddpackage(
      NAME
      cereal
      GITHUB_REPOSITORY
      "USCiLab/cereal"
      GIT_TAG
      "v1.3.2"
    )
  else()
    message(STATUS "Found native cereal::cereal")
  endif()

  if(NOT TARGET Qt${QT_VERSION_MAJOR}::Core AND NOT TARGET Qt${QT_VERSION_MAJOR}::Core)
    message(STATUS "Qt${QT_VERSION_MAJOR} not found, building without Qt support")
  else()
    message(STATUS "Found native Qt${QT_VERSION_MAJOR} ")
    message(STATUS "Headers are in ${Qt6Core_INCLUDE_DIRS}")
  endif()

  if(NOT TARGET dlib::dlib)
    message(STATUS "dlib not found, building without dlib support")
  else()
    message(STATUS "Found native dlib::dlib ")
  endif()

#  if(NOT TARGET tools::tools)
#    cpmaddpackage("gh:lefticus/tools#update_build_system")
#  endif()

endfunction()
