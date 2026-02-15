if (NOT TARGET leaf-core)
    message(FATAL_ERROR "leaf-plugin-file-picker requires leaf-core target")
endif()

set(LEAF_FILE_PICKER_SOURCES
        LFFilePicker.cpp
)

if (LEAF_PLATFORM_DEF STREQUAL "DESKTOP")
    list(APPEND LEAF_FILE_PICKER_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../desktop/LFFilePickerDesktop.cpp
    )
elseif (LEAF_PLATFORM_DEF STREQUAL "ANDROID")
    list(APPEND LEAF_FILE_PICKER_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../android/LFFilePickerAndroid.cpp
    )
endif()

add_library(leaf-plugin-file-picker STATIC
        ${LEAF_FILE_PICKER_SOURCES}
)

set_target_properties(leaf-plugin-file-picker PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(leaf-plugin-file-picker
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(leaf-plugin-file-picker
        PUBLIC
        leaf-core
)
