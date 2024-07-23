
add_library (Ravl2
        Index.cc Index.hh
        Array.hh Array.cc
        ArrayIterZip.hh
        ScanWindow.hh
        Assert.hh
)

set_property(TARGET Ravl2 PROPERTY CXX_STANDARD 20)
target_link_libraries(Ravl2 PUBLIC ${OpenCV_LIBS} fmt::fmt xtensor xtensor-blas spdlog::spdlog lapack )
target_include_directories(Ravl2 PUBLIC "..")

add_subdirectory(Image)
add_subdirectory(Geometry)
