# Video module CMake file
# Created on September 6, 2025

# Check for FFmpeg libraries
option(WITH_FFMPEG "Build with FFmpeg media container support" ON)

if(WITH_FFMPEG)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(FFMPEG QUIET
            libavformat
            libavcodec
            libavutil
            libswscale
            libswresample
        )
    endif()

    if(FFMPEG_FOUND)
        message(STATUS "Found FFmpeg: Building FFmpeg media container support")
        set(HAVE_FFMPEG 1)
        add_compile_definitions(HAVE_FFMPEG=1)
    else()
        message(STATUS "FFmpeg not found: FFmpeg media container support will be disabled")
        set(WITH_FFMPEG OFF)
    endif()
endif()

# Define the Video library with core components
set(VIDEO_SOURCES
    VideoTypes.cc VideoTypes.hh
    Frame.cc Frame.hh
    VideoFrame.cc VideoFrame.hh
    AudioChunk.cc AudioChunk.hh
    MetaDataFrame.cc MetaDataFrame.hh
    MediaContainer.cc MediaContainer.hh
    MemoryMediaContainer.cc MemoryMediaContainer.hh
    StreamIterator.cc StreamIterator.hh
    VideoIO.cc
)

# Add FFmpeg components if available
if(WITH_FFMPEG)
    list(APPEND VIDEO_SOURCES
        FfmpegMediaContainer.cc FfmpegMediaContainer.hh
        FfmpegStreamIterator.cc FfmpegStreamIterator.hh
    )
endif()

# Define the Video library
add_library(Ravl2Video ${VIDEO_SOURCES})

if(WITH_FFMPEG AND FFMPEG_FOUND)
    # Add include directories for FFmpeg
    target_include_directories(Ravl2Video PUBLIC ${FFMPEG_INCLUDE_DIRS})
    # Add link directories for FFmpeg if needed
    target_link_directories(Ravl2Video PUBLIC ${FFMPEG_LIBRARY_DIRS})
endif()

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

# Add FFmpeg libraries if available
if(WITH_FFMPEG)
    target_link_libraries(Ravl2Video
        PRIVATE
            ${FFMPEG_LIBRARIES}
    )
    target_include_directories(Ravl2Video
        PRIVATE
            ${FFMPEG_INCLUDE_DIRS}
    )
    target_compile_options(Ravl2Video
        PRIVATE
            ${FFMPEG_CFLAGS}
    )
endif()

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

add_subdirectory(tests)