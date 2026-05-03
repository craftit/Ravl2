
if(NOT TARGET imguizmo)

    # Pin to the last commit before upstream moved sources into src/ and
    # added a root CMakeLists.txt that defines a competing `imguizmo` target
    # without linking to our local imgui. See ff287e7 "Delete ImGuizmo.cpp".
    FetchContent_Declare(imguizmo_external
            GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo
            GIT_TAG ff3c0732fbec33bc798ecb761bddadfae2763514
            EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(imguizmo_external)

    set(IMGUIZMO_DIR ${imguizmo_external_SOURCE_DIR})

    # Build a minimal static library with ImPlot sources.
    set(_IMGUIZMO_SRC
            ${IMGUIZMO_DIR}/GraphEditor.cpp
            ${IMGUIZMO_DIR}/ImCurveEdit.cpp
            ${IMGUIZMO_DIR}/ImGradient.cpp
            ${IMGUIZMO_DIR}/ImGuizmo.cpp
            ${IMGUIZMO_DIR}/ImSequencer.cpp
    )

    add_library(imguizmo STATIC ${_IMGUIZMO_SRC})

    # ImPlot requires ImGui
    target_link_libraries(imguizmo PUBLIC imgui)

    target_include_directories(imguizmo PUBLIC "${IMGUIZMO_DIR}")

    # Relax a few warnings for 3rd-party code if your project treats warnings as errors.
    if(MSVC)
        target_compile_options(imguizmo PRIVATE /wd4996)
    else()
        # Set compile flags for imguizmo. Suppress old-style cast warnings
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(imguizmo PRIVATE -Wno-old-style-cast -Wno-useless-cast)
        else ()
            target_compile_options(imguizmo PRIVATE -Wno-old-style-cast)
        endif()
    endif()

    set(IMGUIZMO_FOUND TRUE)

endif()
