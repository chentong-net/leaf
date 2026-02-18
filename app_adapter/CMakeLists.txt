if (NOT TARGET leaf-core)
    message(FATAL_ERROR "app-adapter requires leaf-core target")
endif()

set(EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../examples)
set(PLUGINS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../plugins)

add_subdirectory(${EXAMPLES_DIR}/my_profile my_profile_build)
add_subdirectory(${EXAMPLES_DIR}/reader_app reader_app_build)
add_subdirectory(${PLUGINS_DIR}/file_picker_leaf_plugin file_picker_leaf_plugin_build)

add_library(app-adapter STATIC
        AppLaunch.cpp
)

set_target_properties(app-adapter PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(app-adapter
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(app-adapter
        PRIVATE
        leaf-core
        reader-app
        my-profile
        leaf-plugin-file-picker
)
