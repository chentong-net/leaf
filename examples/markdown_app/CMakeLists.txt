if (NOT TARGET leaf-core)
    message(FATAL_ERROR "markdown-app requires leaf-core target")
endif()

if (NOT TARGET markdown)
    message(FATAL_ERROR "markdown-app requires markdown target")
endif()

add_library(markdown-app STATIC
        MarkdownApp.cpp
)

set_target_properties(markdown-app PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(markdown-app
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(markdown-app
        PUBLIC
        leaf-core
        markdown
)
