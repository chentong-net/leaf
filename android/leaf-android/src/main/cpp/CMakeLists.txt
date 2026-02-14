cmake_minimum_required(VERSION 3.22.1)

project("leaf-android")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LEAF_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../core)

set(READER_APP_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../examples/reader_app)
set(PROFILE_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../examples/my_profile)

set(LEAF_PLATFORM_DEF "ANDROID")
add_subdirectory(${LEAF_CORE_DIR} leaf_core_build)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${READER_APP_EXAMPLE_DIR}
        ${PROFILE_EXAMPLE_DIR}
)

add_library(${CMAKE_PROJECT_NAME} SHARED
        ${READER_APP_EXAMPLE_DIR}/ReaderApp.cpp
        ${READER_APP_EXAMPLE_DIR}/BookshelfPage.cpp
        ${READER_APP_EXAMPLE_DIR}/ReaderPage.cpp
        ${READER_APP_EXAMPLE_DIR}/TextSplitter.cpp
        ${PROFILE_EXAMPLE_DIR}/ProfilePage.cpp
        leaf_engine_jni.cpp
        engine_bridge.cpp
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        leaf-core
        GLESv3
        android
        log
        m
)
