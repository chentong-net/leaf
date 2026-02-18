if (NOT TARGET leaf-core)
    message(FATAL_ERROR "file-picker requires leaf-core target")
endif()

if (LEAF_PLATFORM_DEF STREQUAL "DESKTOP")
    add_library(file-picker STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/desktop/LFFilePickerDesktop.cpp
    )
else()
    add_library(file-picker STATIC
            LFFilePicker.cpp
    )
endif()

set_target_properties(file-picker PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(file-picker
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(file-picker
        PRIVATE
        leaf-core
)
