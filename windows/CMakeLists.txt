cmake_minimum_required(VERSION 3.22.1)

project("leaf-windows")

if(NOT WIN32)
    message(FATAL_ERROR "leaf-windows can only be built on Windows")
endif()

add_compile_definitions(LEAF_USE_GL3 WIN32_LEAN_AND_MEAN NOMINMAX)

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

set(LEAF_PLATFORM_DEF "WINDOWS")
set(LEAF_USE_GL3 ON)
add_subdirectory(${LEAF_CORE_DIR} leaf_core_build)
add_subdirectory(${APPLICATION_DIR} application_build)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GLFW_DIR}/include
)

add_executable(leaf-windows
        main.cpp
)

target_link_libraries(leaf-windows PRIVATE
        glfw
        leaf-core
        leaf-application
)

set(LEAF_ASSETS_OUTPUT_DIR "$<TARGET_FILE_DIR:leaf-windows>/assets")
add_custom_target(leaf-windows-prepare-assets
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${LEAF_ASSETS_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LEAF_ASSETS_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${APPLICATION_DIR}/assets" "${LEAF_ASSETS_OUTPUT_DIR}"
        VERBATIM
        COMMENT "Preparing Leaf Windows assets"
)
add_dependencies(leaf-windows leaf-windows-prepare-assets)

enable_testing()
set(LEAF_LOCAL_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tests)
if(EXISTS "${LEAF_LOCAL_TESTS_DIR}/CMakeLists.txt")
    add_subdirectory(${LEAF_LOCAL_TESTS_DIR} tests_build)
endif()
