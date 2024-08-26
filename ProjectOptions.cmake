include(cmake/SystemLink.cmake)
include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


macro(RAVL2_supports_sanitizers)
  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)
    set(SUPPORTS_UBSAN ON)
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    set(SUPPORTS_ASAN ON)
  endif()
endmacro()

macro(RAVL2_setup_options)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "Default")
    option(RAVL2_ENABLE_HARDENING "Enable hardening" ON)
  else()
    option(RAVL2_ENABLE_HARDENING "Enable hardening" OFF)
  endif()
  option(RAVL2_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(
          RAVL2_ENABLE_GLOBAL_HARDENING
          "Attempt to push hardening options to built dependencies"
          ON
          RAVL2_ENABLE_HARDENING
          OFF)

  RAVL2_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR RAVL2_PACKAGING_MAINTAINER_MODE)
    option(RAVL2_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(RAVL2_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(RAVL2_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(RAVL2_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(RAVL2_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(RAVL2_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(RAVL2_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(RAVL2_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(RAVL2_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(RAVL2_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(RAVL2_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(RAVL2_ENABLE_PCH "Enable precompiled headers" OFF)
    option(RAVL2_ENABLE_CACHE "Enable ccache" OFF)
  else()
    # LTO seems to stop debug builds from working
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "Default")
      option(RAVL2_ENABLE_IPO "Enable IPO/LTO" OFF)
    else()
      option(RAVL2_ENABLE_IPO "Enable IPO/LTO" ON)
    endif()
    option(RAVL2_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(RAVL2_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(RAVL2_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF) #  ${SUPPORTS_ASAN}
    option(RAVL2_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(RAVL2_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})  #
    option(RAVL2_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(RAVL2_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(RAVL2_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(RAVL2_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(RAVL2_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(RAVL2_ENABLE_PCH "Enable precompiled headers" OFF)
    option(RAVL2_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      RAVL2_ENABLE_IPO
      RAVL2_WARNINGS_AS_ERRORS
      RAVL2_ENABLE_USER_LINKER
      RAVL2_ENABLE_SANITIZER_ADDRESS
      RAVL2_ENABLE_SANITIZER_LEAK
      RAVL2_ENABLE_SANITIZER_UNDEFINED
      RAVL2_ENABLE_SANITIZER_THREAD
      RAVL2_ENABLE_SANITIZER_MEMORY
      RAVL2_ENABLE_UNITY_BUILD
      RAVL2_ENABLE_CLANG_TIDY
      RAVL2_ENABLE_CPPCHECK
      RAVL2_ENABLE_COVERAGE
      RAVL2_ENABLE_PCH
      RAVL2_ENABLE_CACHE)
  endif()

  RAVL2_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
  if(LIBFUZZER_SUPPORTED AND (RAVL2_ENABLE_SANITIZER_ADDRESS OR RAVL2_ENABLE_SANITIZER_THREAD OR RAVL2_ENABLE_SANITIZER_UNDEFINED))
    set(DEFAULT_FUZZER ON)
  else()
    set(DEFAULT_FUZZER OFF)
  endif()

  option(RAVL2_BUILD_FUZZ_TESTS "Enable fuzz testing executable" ${DEFAULT_FUZZER})
  option(RAVL2_BUILD_EXAMPLES "Enable building examples" ON)
endmacro()

macro(RAVL2_global_options)
  if(RAVL2_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    RAVL2_enable_ipo()
  endif()

  RAVL2_supports_sanitizers()

  if(RAVL2_ENABLE_HARDENING AND RAVL2_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR RAVL2_ENABLE_SANITIZER_UNDEFINED
       OR RAVL2_ENABLE_SANITIZER_ADDRESS
       OR RAVL2_ENABLE_SANITIZER_THREAD
       OR RAVL2_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${RAVL2_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${RAVL2_ENABLE_SANITIZER_UNDEFINED}")
    RAVL2_enable_hardening(RAVL2_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(RAVL2_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(RAVL2_warnings INTERFACE)
  add_library(RAVL2_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  RAVL2_set_project_warnings(
    RAVL2_warnings
    ${RAVL2_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  if(RAVL2_ENABLE_USER_LINKER)
    include(cmake/Linker.cmake)
    RAVL2_configure_linker(RAVL2_options)
  endif()

  include(cmake/Sanitizers.cmake)
  RAVL2_enable_sanitizers(
    RAVL2_options
    ${RAVL2_ENABLE_SANITIZER_ADDRESS}
    ${RAVL2_ENABLE_SANITIZER_LEAK}
    ${RAVL2_ENABLE_SANITIZER_UNDEFINED}
    ${RAVL2_ENABLE_SANITIZER_THREAD}
    ${RAVL2_ENABLE_SANITIZER_MEMORY})

  set_target_properties(RAVL2_options PROPERTIES UNITY_BUILD ${RAVL2_ENABLE_UNITY_BUILD})

  if(RAVL2_ENABLE_PCH)
    target_precompile_headers(
      RAVL2_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(RAVL2_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    RAVL2_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(RAVL2_ENABLE_CLANG_TIDY)
    RAVL2_enable_clang_tidy(RAVL2_options ${RAVL2_WARNINGS_AS_ERRORS})
  endif()

  if(RAVL2_ENABLE_CPPCHECK)
    RAVL2_enable_cppcheck(${RAVL2_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(RAVL2_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    RAVL2_enable_coverage(RAVL2_options)
  endif()

  if(RAVL2_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(RAVL2_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(RAVL2_ENABLE_HARDENING AND NOT RAVL2_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR RAVL2_ENABLE_SANITIZER_UNDEFINED
       OR RAVL2_ENABLE_SANITIZER_ADDRESS
       OR RAVL2_ENABLE_SANITIZER_THREAD
       OR RAVL2_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    RAVL2_enable_hardening(RAVL2_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
