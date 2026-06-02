if (NOT TARGET leaf-core)
    message(FATAL_ERROR "radio-group requires leaf-core target")
endif()

add_library(radio-group STATIC
        LFRadioGroup.cpp
)

set_target_properties(radio-group PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(radio-group
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(radio-group
        PRIVATE
        leaf-core
)
