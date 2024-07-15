cmake_minimum_required (VERSION 2.6)

project (Ravl2)

include_directories ("..")

find_package(xtensor REQUIRED)

#add_subdirectory(Type)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17 -O2 -DNDEBUG" )
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -O " )

add_library (Ravl2 RefCounter.cc Index.cc EdgeSobel.cc RefCounterWrap.cc) 

add_executable(testRavl2Core testRavl2Core.cc )
add_executable(benchmarkAccess  benchmarkAccess.cc )
target_link_libraries (benchmarkAccess PUBLIC xtensor)

target_link_libraries (testRavl2Core Ravl2)
