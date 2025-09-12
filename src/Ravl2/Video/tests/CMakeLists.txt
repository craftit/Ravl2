# CMakeLists.txt for Video module tests

# Create the tests executable
add_executable(test_video_module
    test_MemoryMediaContainer.cc
    test_VideoFrame.cc
    test_VideoIO.cc
)

# Link with the necessary libraries
target_link_libraries(test_video_module
    PRIVATE
    Ravl2Video
    Ravl2Core
    Catch2::Catch2WithMain
)

target_compile_definitions(test_video_module PRIVATE -DRAVL_SOURCE_DIR="${PROJECT_SOURCE_DIR}" -DRAVL_BINARY_DIR="${PROJECT_BINARY_DIR}" -DRAVL_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

# Add the test to CTest
add_test(
    NAME test_video_module
    COMMAND test_video_module
)

# Create the exMediaContainer example
add_executable(exMediaContainer
    exMediaContainer.cc
)

# Link with the necessary libraries
target_link_libraries(exMediaContainer
    PRIVATE
    Ravl2Video
    Ravl2OpenCV
)
