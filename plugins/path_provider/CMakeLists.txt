if (NOT TARGET leaf-core)
    message(FATAL_ERROR "path-provider requires leaf-core target")
endif()

if (LEAF_PLATFORM_DEF STREQUAL "WINDOWS" OR
    LEAF_PLATFORM_DEF STREQUAL "MACOS")
    if (NOT DEFINED LEAF_APPLICATION_ID OR
        LEAF_APPLICATION_ID STREQUAL "")
        message(FATAL_ERROR
                "path-provider requires LEAF_APPLICATION_ID")
    endif()
endif()

if (LEAF_PLATFORM_DEF STREQUAL "WINDOWS")
    add_library(path-provider STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/windows/LFPathProviderWindows.cpp
    )
elseif (LEAF_PLATFORM_DEF STREQUAL "MACOS")
    add_library(path-provider STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/macos/LFPathProviderMacOS.cpp
    )
elseif (LEAF_PLATFORM_DEF STREQUAL "WEB")
    add_library(path-provider STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/web/LFPathProviderWeb.cpp
    )
else()
    add_library(path-provider STATIC
            LFPathProvider.cpp
    )
endif()

set_target_properties(path-provider PROPERTIES POSITION_INDEPENDENT_CODE ON)

if (LEAF_PLATFORM_DEF STREQUAL "WINDOWS" OR
    LEAF_PLATFORM_DEF STREQUAL "MACOS")
    target_compile_definitions(path-provider PRIVATE
            "LEAF_APPLICATION_ID=\"${LEAF_APPLICATION_ID}\""
    )
endif()

target_include_directories(path-provider
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(path-provider
        PRIVATE
        leaf-core
)
