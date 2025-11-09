include(cmake/CPM.cmake)

# Options controlling optional stacks and minimal deps
option(RAVL2_MIN_DEPS "Disable optional stacks (Qt, dlib, Display) for a minimal build" OFF)
option(RAVL2_ENABLE_DISPLAY_STACK "Enable SDL2+bgfx+ImGui display stack (optional)" ON)
# Note: Vulkan will be preferred as default renderer when available; on macOS use MoltenVK/Metal fallback.

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
    #set(BLA_VENDOR "All")
  endif()

  find_package(BLAS REQUIRED)
  message(STATUS "BLAS_LIBS: ${BLAS_LIBRARIES}")
  message(STATUS "BLA_VENDOR: ${BLA_VENDOR}")

  find_package(LAPACK REQUIRED)
  message(STATUS "LAPACK_LIBRARIES: ${LAPACK_LIBRARIES}")

  # Try and use native packages if they're available

  find_package(fmt QUIET)
  find_package(spdlog QUIET)
  find_package(Eigen3 5.0 QUIET)
  find_package(Catch2 3 QUIET)
  find_package(CLI11 QUIET)
  find_package(nlohmann_json QUIET)
  find_package(cereal QUIET)
  find_package(OpenGL QUIET)

  # Optional dependencies, we won't build them if they're not found

  find_package(dlib QUIET)
  find_package(glfw3 QUIET)

  if(NOT RAVL2_MIN_DEPS)
    find_package(QT NAMES Qt6 Qt5 QUIET COMPONENTS Widgets)
    if(QT_FOUND)
      find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)
    else()
      message(STATUS "Qt not found or disabled, building without Qt support")
    endif()
  endif()


  # For each dependency, see if it's
  # already been provided to us by a parent project

  if(NOT TARGET fmt::fmt)
    cpmaddpackage("gh:fmtlib/fmt#12.1.0")
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
      1.16.0
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
            5.0.0
            GIT_TAG
            "5.0.0"
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

  # Optional Display Stack (SDL2 + bgfx + ImGui + ImPlot), minimal wiring
  if(RAVL2_ENABLE_DISPLAY_STACK AND NOT RAVL2_MIN_DEPS)
    message(STATUS "Ravl2 Display Stack: enabled")

    # Prefer native SDL2 and Vulkan first
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
      message(STATUS "Found native SDL2: ${SDL2_VERSION}")
    else()
      message(STATUS "SDL2 not found in system packages; display stack will require SDL2 later or add via CPM in a follow-up.")
    endif()

    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
      message(STATUS "Found native Vulkan: ${Vulkan_LIBRARY}")
    else()
      if(APPLE)
        message(STATUS "Vulkan not found on macOS; expecting MoltenVK (system install). Will fall back to Metal at runtime if needed.")
      else()
        message(STATUS "Vulkan not found; OpenGL/Direct3D fallback may be used at runtime.")
      endif()
    endif()

    # bgfx stack (CPM) — using bgfx.cmake which provides CMake build system for bgfx
    cpmaddpackage(
      NAME bgfx.cmake
      GITHUB_REPOSITORY bkaradzic/bgfx.cmake
      GIT_TAG master
      OPTIONS
        "BGFX_BUILD_EXAMPLES OFF"
        "BGFX_BUILD_TOOLS OFF"
        "BGFX_INSTALL OFF"
    )

    # Dear ImGui (docking) and ImPlot
    cpmaddpackage(
      NAME imgui
      GITHUB_REPOSITORY ocornut/imgui
      GIT_TAG docking
    )

    cpmaddpackage(
      NAME implot
      GITHUB_REPOSITORY epezent/implot
      GIT_TAG v0.16
    )

    # Backend preference cache var (default Vulkan)
    set(RAVL2_BGFX_BACKEND "Vulkan" CACHE STRING "Default bgfx backend: Auto, Vulkan, Metal, D3D12, D3D11, OpenGL")
    set_property(CACHE RAVL2_BGFX_BACKEND PROPERTY STRINGS Auto Vulkan Metal D3D12 D3D11 OpenGL)
    message(STATUS "Ravl2 Display: default backend is ${RAVL2_BGFX_BACKEND}")
  else()
    if(RAVL2_MIN_DEPS)
      message(STATUS "RAVL2_MIN_DEPS=ON: Optional stacks disabled (Qt, dlib, Display)")
    else()
      message(STATUS "Ravl2 Display Stack: disabled (set RAVL2_ENABLE_DISPLAY_STACK=ON to enable)")
    endif()
  endif()

endfunction()
