
if(NOT TARGET imgui)

    # Make sure we support glfw3 it is available, but don't require it.
    find_package(glfw3 QUIET)
    find_package(SDL2 QUIET)

    FetchContent_Declare(imgui_external
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG docking
            EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(imgui_external)

    set(IMGUI_DIR ${imgui_external_SOURCE_DIR})

    FetchContent_Declare(imguizmo_external
            GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
            EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(imguizmo_external)

    set(IMGUIZMO_DIR ${imguizmo_external_SOURCE_DIR})


    # Build a minimal static library with ImGui core sources.
    set(_IMGUI_SRC
            ${IMGUI_DIR}/imgui.cpp
            ${IMGUI_DIR}/imgui_draw.cpp
            ${IMGUI_DIR}/imgui_tables.cpp
            ${IMGUI_DIR}/imgui_widgets.cpp
            # Demo is optional but useful during development; comment out to reduce size.
            ${IMGUI_DIR}/imgui_demo.cpp
            ${IMGUIZMO_DIR}/ImGuizmo.cpp
    )

    if(SDL2_FOUND)
        message(STATUS "Including SDL2 support in imgui")
        list(APPEND _IMGUI_SRC ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
                                ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
        )
    endif ()

    message(STATUS "Including OpenGL2 support in imgui")
    list(APPEND _IMGUI_SRC ${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp
    )

    if(GLFW_FOUND)
        message(STATUS "Including glfw support in imgui")
        list(APPEND _IMGUI_SRC ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        )
    endif ()

    add_library(imgui STATIC ${_IMGUI_SRC})

    if(SDL2_FOUND)
        target_link_libraries(imgui PRIVATE SDL2::SDL2)
    endif ()


    target_include_directories(imgui PUBLIC "${IMGUI_DIR}" "${IMGUI_DIR}/backends")
    # Relax a few warnings for 3rd-party code if your project treats warnings as errors.
    if(MSVC)
        target_compile_options(imgui PRIVATE /wd4996)
    else()
        # Set compile flags for imgui.  Suppress useless cast
        # If we're using gcc
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(imgui PRIVATE -Wno-old-style-cast -Wno-useless-cast)
        else ()
            target_compile_options(imgui PRIVATE -Wno-old-style-cast)
        endif()
    endif()
    set(IMGUI_FOUND TRUE)

    target_include_directories(imgui PUBLIC
            ${imgui_external_SOURCE_DIR}
            ${imguizmo_external_SOURCE_DIR}
    )

    if(GLFW_FOUND)
        target_link_libraries(imgui PUBLIC ${GLFW_LIBRARIES} )
    endif()

endif()