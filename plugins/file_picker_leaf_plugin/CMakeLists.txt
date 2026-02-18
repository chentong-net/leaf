if (NOT TARGET leaf-core)
    message(FATAL_ERROR "file-picker-leaf-plugin requires leaf-core target")
endif()

add_library(leaf-plugin-file-picker STATIC
        LFFilePicker.cpp
)

set_target_properties(leaf-plugin-file-picker PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(leaf-plugin-file-picker
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(leaf-plugin-file-picker
        PRIVATE
        leaf-core
)
