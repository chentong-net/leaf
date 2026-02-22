cmake_minimum_required(VERSION 3.22.1)

project("leaf-native-ios")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT APPLE)
    message(FATAL_ERROR "leaf-native-ios can only be built on Apple toolchains")
endif()

set(LEAF_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
set(LEAF_CORE_DIR ${LEAF_ROOT_DIR}/core)
set(APP_ADAPTER_DIR ${LEAF_ROOT_DIR}/app_adapter)

# Reuse the same C++ module graph as other platforms.
set(LEAF_PLATFORM_DEF "IOS")
add_subdirectory(${LEAF_CORE_DIR} leaf_core_build)
add_subdirectory(${APP_ADAPTER_DIR} app_adapter_build)

add_custom_target(leaf-native-all
        DEPENDS
        leaf-core
        app-adapter
)
