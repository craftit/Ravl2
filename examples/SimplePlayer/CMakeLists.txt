cmake_minimum_required(VERSION 3.21)

project(SimplePlayer VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# Enable automatic handling of Qt resources and UI files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add the executable
add_executable(SimplePlayer
    main.cpp
    MainWindow.cpp
    MainWindow.h
    VideoPlayer.cpp
    VideoPlayer.h
)

# Link Qt and Ravl2 libraries
target_link_libraries(SimplePlayer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Ravl2Video
)

# Set installation target
install(TARGETS SimplePlayer
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin
)
