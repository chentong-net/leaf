if (NOT TARGET leaf-core)
    message(FATAL_ERROR "checkbox requires leaf-core target")
endif()

add_library(checkbox STATIC
        LFCheckbox.cpp
)

set_target_properties(checkbox PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(checkbox
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(checkbox
        PRIVATE
        leaf-core
)
