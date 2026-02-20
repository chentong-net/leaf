if (NOT TARGET leaf-core)
    message(FATAL_ERROR "path-provider requires leaf-core target")
endif()

add_library(path-provider STATIC
        LFPathProvider.cpp
)

set_target_properties(path-provider PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(path-provider
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(path-provider
        PRIVATE
        leaf-core
)
