
add_executable(benchmarkAccess benchmarkAccess.cc)
target_link_libraries (benchmarkAccess PUBLIC Ravl2Core )

add_executable(exAssert exAssert.cc)
target_link_libraries (exAssert PUBLIC Ravl2Core )

add_executable(exCorner exCorner.cc)
target_link_libraries (exCorner PUBLIC Ravl2Image Ravl2OpenCV CLI11::CLI11)

add_executable(extrema extrema.cc)
target_link_libraries (extrema PUBLIC Ravl2Image Ravl2ImageSegmentation Ravl2OpenCV CLI11::CLI11)

find_package(CLI11 CONFIG REQUIRED)
add_executable(intro intro.cc)
target_link_libraries (intro PUBLIC Ravl2Core CLI11::CLI11)
