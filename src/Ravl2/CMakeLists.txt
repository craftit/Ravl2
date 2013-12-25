cmake_minimum_required (VERSION 2.6)

project (Ravl2)

include_directories ("..") 

add_subdirectory(Type)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11 " )

add_library (Ravl2 RefCounter.cc Index.cc EdgeSobel.cc RefCounterWrap.cc) 

add_executable(testRavl2Core testRavl2Core.cc )

target_link_libraries (testRavl2Core Ravl2)
