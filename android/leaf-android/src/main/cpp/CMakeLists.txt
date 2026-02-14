cmake_minimum_required(VERSION 3.22.1)

project("leaf-android")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LEAF_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../core)
set(EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../examples)

set(LEAF_PLATFORM_DEF "ANDROID")
add_subdirectory(${LEAF_CORE_DIR} leaf_core_build)
add_subdirectory(${EXAMPLES_DIR}/my_profile my_profile_build)
add_subdirectory(${EXAMPLES_DIR}/reader_app reader_app_build)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(${CMAKE_PROJECT_NAME} SHARED
        leaf_engine_jni.cpp
        engine_bridge.cpp
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        leaf-core
        reader-app
        my-profile
        GLESv3
        android
        log
        m
)
