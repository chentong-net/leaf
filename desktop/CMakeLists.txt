cmake_minimum_required(VERSION 3.22.1)

project("leaf-desktop")

add_compile_definitions(__DESKTOP__)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LEAF_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core)
set(APP_ADAPTER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../app_adapter)
set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)
set(GLFW_DIR ${DEPS_DIR}/glfw)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_DIR} glfw_build)

set(LEAF_PLATFORM_DEF "DESKTOP")
add_subdirectory(${LEAF_CORE_DIR} leaf_core_build)
add_subdirectory(${APP_ADAPTER_DIR} app_adapter_build)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GLFW_DIR}/include
)

add_executable(leaf-desktop
        desktop_main.cpp
)

#target_include_directories(leaf-desktop PRIVATE ${GLFW_DIR}/include)
target_link_libraries(leaf-desktop PRIVATE
        glfw
        leaf-core
        app-adapter
)

#if(APPLE)
#    target_link_libraries(leaf-desktop PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
#endif()
