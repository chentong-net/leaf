if (NOT TARGET leaf-core)
    message(FATAL_ERROR "leaf-application requires leaf-core target")
endif()

set(EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../examples)
set(PLUGINS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../plugins)

add_subdirectory(${EXAMPLES_DIR}/my_profile my_profile_build)
add_subdirectory(${EXAMPLES_DIR}/component_lab component_lab_build)
add_subdirectory(${EXAMPLES_DIR}/reader_app reader_app_build)
add_subdirectory(${PLUGINS_DIR}/checkbox checkbox_build)
add_subdirectory(${PLUGINS_DIR}/file_picker file_picker_build)
add_subdirectory(${PLUGINS_DIR}/i18n i18n_build)
add_subdirectory(${PLUGINS_DIR}/path_provider path_provider_build)
add_subdirectory(${PLUGINS_DIR}/radio_group radio_group_build)
add_subdirectory(${PLUGINS_DIR}/audio_player audio_player_build)

add_library(leaf-application STATIC
        AppLaunch.cpp
)

set_target_properties(leaf-application PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(leaf-application
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(leaf-application
        PRIVATE
        leaf-core
        component-lab
        reader-app
        my-profile
        checkbox
        file-picker
        i18n
        path-provider
        radio-group
        audio-player
)
