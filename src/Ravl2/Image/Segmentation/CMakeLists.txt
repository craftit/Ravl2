

add_library(Ravl2ImageSegmentation STATIC
        Crack.cc Crack.hh
        CrackCode.cc CrackCode.hh
        Segmentation.hh Segmentation.cc
)

set_property(TARGET Ravl2ImageSegmentation PROPERTY CXX_STANDARD 20)

target_link_libraries(Ravl2ImageSegmentation PUBLIC Ravl2Image)
