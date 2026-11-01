cmake_minimum_required(VERSION 3.22.1)

project("leaf-web")

add_compile_definitions(__WEB__)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LEAF_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core)
set(APPLICATION_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../application)

set(LEAF_PLATFORM_DEF "WEB")
add_subdirectory(${LEAF_CORE_DIR} leaf_core_build)
add_subdirectory(${APPLICATION_DIR} application_build)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_link_options(
        -sUSE_WEBGL2=1
        -sFULL_ES3=1
        -sALLOW_MEMORY_GROWTH=1   # 允许内存动态增长
        -sFETCH=1
        # -sASYNCIFY=1            # 如果后面需要处理复杂的 QuickJS 异步，可能需要开启
)

add_executable(leaf-web
        web_main.cpp
)

target_link_libraries(leaf-web PRIVATE
        leaf-application
)

target_link_options(leaf-web PRIVATE
        $<$<CONFIG:Debug>:-sASSERTIONS=1>
        $<$<NOT:$<CONFIG:Debug>>:-sASSERTIONS=0>
)

set_target_properties(leaf-web PROPERTIES SUFFIX ".html")

set(LEAF_WEB_ASSET_SOURCE_DIR "${APPLICATION_DIR}/assets")
set(LEAF_WEB_ASSET_OUTPUT_DIR "$<TARGET_FILE_DIR:leaf-web>/assets")

add_custom_target(leaf-web-prepare-assets ALL
        COMMAND ${CMAKE_COMMAND}
                "-DLEAF_WEB_ASSET_SOURCE_DIR=${LEAF_WEB_ASSET_SOURCE_DIR}"
                "-DLEAF_WEB_ASSET_OUTPUT_DIR=${LEAF_WEB_ASSET_OUTPUT_DIR}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/prepare_assets.cmake"
        COMMENT "Preparing Leaf Web on-demand assets"
        VERBATIM
)

add_dependencies(leaf-web leaf-web-prepare-assets)

target_link_options(leaf-web PRIVATE --preload-file ${APPLICATION_DIR}/assets@/assets)
target_link_options(leaf-web PRIVATE --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html)
