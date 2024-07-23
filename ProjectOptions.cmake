include(cmake/SystemLink.cmake)
include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


macro(ravl2_supports_sanitizers)
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

macro(ravl2_setup_options)
  option(ravl2_ENABLE_HARDENING "Enable hardening" ON)
  option(ravl2_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(
    ravl2_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    ON
    ravl2_ENABLE_HARDENING
    OFF)

  ravl2_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR ravl2_PACKAGING_MAINTAINER_MODE)
    option(ravl2_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(ravl2_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(ravl2_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(ravl2_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(ravl2_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(ravl2_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(ravl2_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(ravl2_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(ravl2_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(ravl2_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(ravl2_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(ravl2_ENABLE_PCH "Enable precompiled headers" OFF)
    option(ravl2_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(ravl2_ENABLE_IPO "Enable IPO/LTO" ON)
    option(ravl2_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(ravl2_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(ravl2_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
    option(ravl2_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(ravl2_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
    option(ravl2_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(ravl2_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(ravl2_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(ravl2_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(ravl2_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
    option(ravl2_ENABLE_PCH "Enable precompiled headers" OFF)
    option(ravl2_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      ravl2_ENABLE_IPO
      ravl2_WARNINGS_AS_ERRORS
      ravl2_ENABLE_USER_LINKER
      ravl2_ENABLE_SANITIZER_ADDRESS
      ravl2_ENABLE_SANITIZER_LEAK
      ravl2_ENABLE_SANITIZER_UNDEFINED
      ravl2_ENABLE_SANITIZER_THREAD
      ravl2_ENABLE_SANITIZER_MEMORY
      ravl2_ENABLE_UNITY_BUILD
      ravl2_ENABLE_CLANG_TIDY
      ravl2_ENABLE_CPPCHECK
      ravl2_ENABLE_COVERAGE
      ravl2_ENABLE_PCH
      ravl2_ENABLE_CACHE)
  endif()

  ravl2_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
  if(LIBFUZZER_SUPPORTED AND (ravl2_ENABLE_SANITIZER_ADDRESS OR ravl2_ENABLE_SANITIZER_THREAD OR ravl2_ENABLE_SANITIZER_UNDEFINED))
    set(DEFAULT_FUZZER ON)
  else()
    set(DEFAULT_FUZZER OFF)
  endif()

  option(ravl2_BUILD_FUZZ_TESTS "Enable fuzz testing executable" ${DEFAULT_FUZZER})
  option(ravl2_BUILD_EXAMPLES "Enable building examples" ON)
endmacro()

macro(ravl2_global_options)
  if(ravl2_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    ravl2_enable_ipo()
  endif()

  ravl2_supports_sanitizers()

  if(ravl2_ENABLE_HARDENING AND ravl2_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR ravl2_ENABLE_SANITIZER_UNDEFINED
       OR ravl2_ENABLE_SANITIZER_ADDRESS
       OR ravl2_ENABLE_SANITIZER_THREAD
       OR ravl2_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${ravl2_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${ravl2_ENABLE_SANITIZER_UNDEFINED}")
    ravl2_enable_hardening(ravl2_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(ravl2_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(ravl2_warnings INTERFACE)
  add_library(ravl2_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  ravl2_set_project_warnings(
    ravl2_warnings
    ${ravl2_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  if(ravl2_ENABLE_USER_LINKER)
    include(cmake/Linker.cmake)
    ravl2_configure_linker(ravl2_options)
  endif()

  include(cmake/Sanitizers.cmake)
  ravl2_enable_sanitizers(
    ravl2_options
    ${ravl2_ENABLE_SANITIZER_ADDRESS}
    ${ravl2_ENABLE_SANITIZER_LEAK}
    ${ravl2_ENABLE_SANITIZER_UNDEFINED}
    ${ravl2_ENABLE_SANITIZER_THREAD}
    ${ravl2_ENABLE_SANITIZER_MEMORY})

  set_target_properties(ravl2_options PROPERTIES UNITY_BUILD ${ravl2_ENABLE_UNITY_BUILD})

  if(ravl2_ENABLE_PCH)
    target_precompile_headers(
      ravl2_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(ravl2_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    ravl2_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(ravl2_ENABLE_CLANG_TIDY)
    ravl2_enable_clang_tidy(ravl2_options ${ravl2_WARNINGS_AS_ERRORS})
  endif()

  if(ravl2_ENABLE_CPPCHECK)
    ravl2_enable_cppcheck(${ravl2_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(ravl2_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    ravl2_enable_coverage(ravl2_options)
  endif()

  if(ravl2_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(ravl2_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(ravl2_ENABLE_HARDENING AND NOT ravl2_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR ravl2_ENABLE_SANITIZER_UNDEFINED
       OR ravl2_ENABLE_SANITIZER_ADDRESS
       OR ravl2_ENABLE_SANITIZER_THREAD
       OR ravl2_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    ravl2_enable_hardening(ravl2_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
