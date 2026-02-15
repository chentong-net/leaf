# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project("leaf-ohos")

add_compile_definitions(__OHOS__)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

set(LEAF_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../core)
set(APP_ADAPTER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../app_adapter)

set(LEAF_PLATFORM_DEF "OHOS")
add_subdirectory(${LEAF_CORE_DIR} leaf_core_build)
add_subdirectory(${APP_ADAPTER_DIR} app_adapter_build)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(leaf_ohos SHARED
        leaf_engine_napi.cpp
        engine_bridge.cpp
)

target_link_libraries(leaf_ohos PUBLIC
        leaf-core
        app-adapter
        libace_ndk.z.so
        libace_napi.z.so
        librawfile.z.so
        libGLESv3.so
        libEGL.so
        libhilog_ndk.z.so
)
