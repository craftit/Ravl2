
add_executable(benchmarkAccess benchmarkAccess.cc)
target_link_libraries (benchmarkAccess PUBLIC Ravl2 )

add_executable(exAssert exAssert.cc)
target_link_libraries (exAssert PUBLIC Ravl2 )
