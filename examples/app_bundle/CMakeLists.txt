if (NOT TARGET leaf-core)
    message(FATAL_ERROR "app-bundle requires leaf-core target")
endif()

add_subdirectory(${EXAMPLES_DIR}/my_profile my_profile_build)
add_subdirectory(${EXAMPLES_DIR}/reader_app reader_app_build)

add_library(app-bundle STATIC
        AppLaunch.cpp
)

set_target_properties(app-bundle PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(app-bundle
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(app-bundle
        PRIVATE
        leaf-core
        reader-app
        my-profile
)
