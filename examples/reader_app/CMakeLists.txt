if (NOT TARGET leaf-core)
    message(FATAL_ERROR "reader-app requires leaf-core target")
endif()

if (NOT TARGET my-profile)
    message(FATAL_ERROR "reader-app requires my-profile target, add_subdirectory(my_profile) first")
endif()

add_library(reader-app STATIC
        ReaderApp.cpp
        BookshelfPage.cpp
        ReaderPage.cpp
        TextSplitter.cpp
)

set_target_properties(reader-app PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(reader-app
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(reader-app
        PUBLIC
        leaf-core
        file-picker
        path-provider
        my-profile
)
