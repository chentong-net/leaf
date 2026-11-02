if (NOT TARGET leaf-core)
    message(FATAL_ERROR "markdown requires leaf-core target")
endif()

add_library(markdown STATIC
        LFMarkdownParser.cpp
        LFMarkdownView.cpp
)

set_target_properties(markdown PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(markdown
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(markdown
        PUBLIC
        leaf-core
)
