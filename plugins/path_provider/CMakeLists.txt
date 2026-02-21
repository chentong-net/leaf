if (NOT TARGET leaf-core)
    message(FATAL_ERROR "path-provider requires leaf-core target")
endif()

if (LEAF_PLATFORM_DEF STREQUAL "WEB")
    add_library(path-provider STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/web/LFPathProviderWeb.cpp
    )
else()
    add_library(path-provider STATIC
            LFPathProvider.cpp
    )
endif()

set_target_properties(path-provider PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(path-provider
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(path-provider
        PRIVATE
        leaf-core
)
