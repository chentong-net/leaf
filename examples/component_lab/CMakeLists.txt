if (NOT TARGET leaf-core)
    message(FATAL_ERROR "component-lab requires leaf-core target")
endif()

if (NOT TARGET my-profile)
    message(FATAL_ERROR "component-lab requires my-profile target, add_subdirectory(my_profile) first")
endif()

add_library(component-lab STATIC
        ComponentLab.cpp
)

set_target_properties(component-lab PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(component-lab
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(component-lab
        PUBLIC
        leaf-core
        my-profile
        checkbox
        audio-player
        i18n
        local-time
        path-provider
        radio-group
)
