cmake_minimum_required(VERSION 3.22.1)

project("leaf-android")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LEAF_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../core)
set(APP_ADAPTER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../app_adapter)

set(LEAF_PLATFORM_DEF "ANDROID")
add_subdirectory(${LEAF_CORE_DIR} leaf_core_build)
add_subdirectory(${APP_ADAPTER_DIR} app_adapter_build)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(${CMAKE_PROJECT_NAME} SHARED
        leaf_engine_jni.cpp
        engine_bridge.cpp
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        leaf-core
        app-adapter
        GLESv3
        android
        log
        m
)
