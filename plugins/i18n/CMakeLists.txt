if (NOT TARGET leaf-core)
    message(FATAL_ERROR "i18n requires leaf-core target")
endif()

if (NOT TARGET path-provider)
    message(FATAL_ERROR "i18n requires path-provider target")
endif()

if (LEAF_PLATFORM_DEF STREQUAL "DESKTOP")
    add_library(i18n STATIC
            LFI18n.cpp
            LFI18nManager.cpp
            desktop/LFI18nPlatformDesktop.cpp
    )
else()
    add_library(i18n STATIC
            LFI18n.cpp
            LFI18nManager.cpp
            LFI18nPlatformNative.cpp
    )
endif()

set_target_properties(i18n PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(i18n
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(i18n
        PRIVATE
        leaf-core
        path-provider
)
