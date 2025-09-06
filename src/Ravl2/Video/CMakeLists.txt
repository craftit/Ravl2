# Video module CMake file
# Created on September 6, 2025

# Define the Video library
add_library(Ravl2Video
    VideoTypes.cc VideoTypes.hh
    Frame.cc Frame.hh
    VideoFrame.cc VideoFrame.hh
    AudioChunk.cc AudioChunk.hh
    MetaDataFrame.cc MetaDataFrame.hh
    MediaContainer.cc MediaContainer.hh
    StreamIterator.cc StreamIterator.hh
)

# Create an alias target with the ravl2:: namespace prefix
add_library(ravl2::Ravl2Video ALIAS Ravl2Video)

# Link with the core library and any other dependencies
target_link_libraries(Ravl2Video
    PUBLIC
        ravl2::Ravl2Core
    PRIVATE
        RAVL2_options
        RAVL2_warnings
)

# Add include directories
target_include_directories(Ravl2Video
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
)

# Set installation targets for the library
install(TARGETS Ravl2Video
    EXPORT Ravl2Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install header files
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/Ravl2/Video
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Ravl2
    FILES_MATCHING PATTERN "*.hh"
)
