if (NOT TARGET leaf-core)
    message(FATAL_ERROR "local-time requires leaf-core target")
endif()

if (LEAF_PLATFORM_DEF STREQUAL "DESKTOP")
    add_library(local-time STATIC
            LFLocalTime.cpp
            desktop/LFLocalTimePlatformDesktop.cpp
    )
else()
    add_library(local-time STATIC
            LFLocalTime.cpp
            LFLocalTimePlatformNative.cpp
    )
endif()

set_target_properties(local-time PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(local-time
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(local-time
        PRIVATE
        leaf-core
)
