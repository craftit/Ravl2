
if(NOT TARGET implot)

    FetchContent_Declare(implot_external
            GIT_REPOSITORY https://github.com/epezent/implot.git
            GIT_TAG master
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

    target_include_directories(implot PUBLIC "${IMPLOT_DIR}")

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
