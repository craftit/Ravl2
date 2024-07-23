
add_executable(benchmarkAccess benchmarkAccess.cc)
target_link_libraries (benchmarkAccess PUBLIC Ravl2Core )

add_executable(exAssert exAssert.cc)
target_link_libraries (exAssert PUBLIC Ravl2Core )

add_executable(exAffine exAffine.cc)
target_link_libraries (exAffine PUBLIC Ravl2Geometry )
