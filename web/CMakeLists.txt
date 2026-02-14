cmake_minimum_required(VERSION 3.22.1)

project("leaf-web")

add_compile_definitions(__WEB__)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LEAF_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core)
set(EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../examples)

set(LEAF_PLATFORM_DEF "WEB")
add_subdirectory(${LEAF_CORE_DIR} leaf_core_build)
add_subdirectory(${EXAMPLES_DIR}/my_profile my_profile_build)
add_subdirectory(${EXAMPLES_DIR}/reader_app reader_app_build)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_link_options(
        -sUSE_WEBGL2=1
        -sFULL_ES3=1
        -sALLOW_MEMORY_GROWTH=1   # 允许内存动态增长
        -sASSERTIONS=1            # 开启断言调试
        -sFETCH=1
        # -sASYNCIFY=1            # 如果后面需要处理复杂的 QuickJS 异步，可能需要开启
)

add_executable(leaf-web
        web_main.cpp
)

target_link_libraries(leaf-web PRIVATE
        leaf-core
        reader-app
        my-profile
)

set_target_properties(leaf-web PROPERTIES SUFFIX ".html")

target_link_options(leaf-web PRIVATE --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/fonts@/fonts)
target_link_options(leaf-web PRIVATE --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html)
