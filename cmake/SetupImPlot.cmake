
if(NOT TARGET implot)

    # Pinned dependency: review and update periodically, see README_dependencies.md 'Pinned dependencies'.
    # Tracks the 'master' branch; pinned 2026-11-28.
    FetchContent_Declare(implot_external
            GIT_REPOSITORY https://github.com/epezent/implot.git
            GIT_TAG 72de27832d1e6299bcc930ce01073d899d420392 # master, 2026-10-25
            EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(implot_external)

    set(IMPLOT_DIR ${implot_external_SOURCE_DIR})

    # Build a minimal static library with ImPlot sources.
    set(_IMPLOT_SRC
            ${IMPLOT_DIR}/implot.cpp
            ${IMPLOT_DIR}/implot_items.cpp
            # Demo is optional but useful during development; comment out to reduce size if needed.
            ${IMPLOT_DIR}/implot_demo.cpp
    )

    add_library(implot STATIC ${_IMPLOT_SRC})

    # ImPlot requires ImGui
    target_link_libraries(implot PUBLIC imgui)

    target_include_directories(implot SYSTEM PUBLIC "${IMPLOT_DIR}")

    # Relax a few warnings for 3rd-party code if your project treats warnings as errors.
    if(MSVC)
        target_compile_options(implot PRIVATE /wd4996)
    else()
        # Set compile flags for implot. Suppress old-style cast warnings
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(implot PRIVATE -Wno-old-style-cast -Wno-useless-cast)
        else ()
            target_compile_options(implot PRIVATE -Wno-old-style-cast)
        endif()
    endif()

    set(IMPLOT_FOUND TRUE)

endif()
