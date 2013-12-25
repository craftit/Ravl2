cmake_minimum_required (VERSION 2.6)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11 " )

add_library (Ravl2Type Scope.cc Namespace.cc Type.cc) 

#add_executable(testRavl2Type testRavl2Type.cc )

#target_link_libraries (testRavl2Core Ravl2)
