if (NOT TARGET leaf-core)
    message(FATAL_ERROR "my-profile requires leaf-core target")
endif()

add_library(my-profile STATIC
        ProfilePage.cpp
)

set_target_properties(my-profile PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(my-profile
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(my-profile
        PUBLIC
        leaf-core
)
