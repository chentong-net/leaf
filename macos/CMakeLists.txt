cmake_minimum_required(VERSION 3.22.1)

project("leaf-macos")

if(NOT APPLE)
    message(FATAL_ERROR "leaf-macos can only be built on macOS")
endif()

add_compile_definitions(LEAF_USE_GL3)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LEAF_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core)
set(APPLICATION_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../application)
set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)
set(GLFW_DIR ${DEPS_DIR}/glfw)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_DIR} glfw_build)

set(LEAF_PLATFORM_DEF "MACOS")
set(LEAF_USE_GL3 ON)
add_subdirectory(${LEAF_CORE_DIR} leaf_core_build)
add_subdirectory(${APPLICATION_DIR} application_build)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GLFW_DIR}/include
)

add_executable(leaf-macos
        main.cpp
)

target_link_libraries(leaf-macos PRIVATE
        glfw
        leaf-core
        leaf-application
)

set(LEAF_ASSETS_OUTPUT_DIR "$<TARGET_FILE_DIR:leaf-macos>/assets")
add_custom_target(leaf-macos-prepare-assets
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${LEAF_ASSETS_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LEAF_ASSETS_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${APPLICATION_DIR}/assets" "${LEAF_ASSETS_OUTPUT_DIR}"
        VERBATIM
        COMMENT "Preparing Leaf macOS assets"
)
add_dependencies(leaf-macos leaf-macos-prepare-assets)

enable_testing()
set(LEAF_LOCAL_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tests)
if(EXISTS "${LEAF_LOCAL_TESTS_DIR}/CMakeLists.txt")
    add_subdirectory(${LEAF_LOCAL_TESTS_DIR} tests_build)
endif()
