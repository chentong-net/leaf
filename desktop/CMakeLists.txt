cmake_minimum_required(VERSION 3.22.1)

project("leaf-desktop")

add_compile_definitions(__DESKTOP__)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LEAF_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core)
set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)
set(GLFW_DIR ${DEPS_DIR}/glfw)

set(READER_APP_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../examples/reader_app)
set(PROFILE_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../examples/my_profile)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_DIR} glfw_build)

set(LEAF_PLATFORM_DEF "DESKTOP")
add_subdirectory(${LEAF_CORE_DIR} leaf_core_build)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${READER_APP_EXAMPLE_DIR}
        ${PROFILE_EXAMPLE_DIR}
        ${GLFW_DIR}/include
)

add_executable(leaf-desktop
        ${READER_APP_EXAMPLE_DIR}/ReaderApp.cpp
        ${READER_APP_EXAMPLE_DIR}/BookshelfPage.cpp
        ${READER_APP_EXAMPLE_DIR}/ReaderPage.cpp
        ${READER_APP_EXAMPLE_DIR}/TextSplitter.cpp
        ${PROFILE_EXAMPLE_DIR}/ProfilePage.cpp
        desktop_main.cpp
)

#target_include_directories(leaf-desktop PRIVATE ${GLFW_DIR}/include)
target_link_libraries(leaf-desktop PRIVATE glfw)
target_link_libraries(leaf-desktop PRIVATE leaf-core)

#if(APPLE)
#    target_link_libraries(leaf-desktop PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
#endif()
