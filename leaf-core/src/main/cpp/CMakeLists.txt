cmake_minimum_required(VERSION 3.22.1)

project("leaf-core")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)
set(QUICKJS_DIR ${DEPS_DIR}/quickjs)
set(NANOVG_DIR ${DEPS_DIR}/nanovg/src)
set(YOGA_DIR ${DEPS_DIR}/yoga)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${QUICKJS_DIR}
        ${NANOVG_DIR}
        ${YOGA_DIR}
)

# 编译QuickJS
# 只编译最核心的文件，避免编译它的命令行工具
add_library(quickjs STATIC
        ${QUICKJS_DIR}/quickjs.c
        ${QUICKJS_DIR}/libregexp.c
        ${QUICKJS_DIR}/libunicode.c
        ${QUICKJS_DIR}/cutils.c
)

target_compile_definitions(quickjs PRIVATE _GNU_SOURCE CONFIG_VERSION="2025-09-13")

set(CORE_SOURCES
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        ${QUICKJS_DIR}/quickjs.c
        ${QUICKJS_DIR}/libregexp.c
        ${QUICKJS_DIR}/libunicode.c
        ${QUICKJS_DIR}/cutils.c
        ${QUICKJS_DIR}/dtoa.c

        ${NANOVG_DIR}/nanovg.c

        ${YOGA_DIR}/Yoga.cpp
        ${YOGA_DIR}/YGConfig.cpp
        ${YOGA_DIR}/YGEnums.cpp
        ${YOGA_DIR}/YGNode.cpp
        ${YOGA_DIR}/YGLayout.cpp
        ${YOGA_DIR}/YGStyle.cpp
        ${YOGA_DIR}/YGValue.cpp
        ${YOGA_DIR}/log.cpp
        ${YOGA_DIR}/Utils.cpp
        ${YOGA_DIR}/YGNodePrint.cpp
        ${YOGA_DIR}/event/event.cpp

        LFEngine.cpp
        LFBox.cpp
        LFLinear.cpp
        LFImage.cpp
        LFNode.cpp
        LFText.cpp

        # Event system (Phase 1: Basic touch events)
        event/LFHitTest.cpp
        event/LFEventDispatcher.cpp
        event/LFGestureArena.cpp

        # Gesture system (Phase 2: Gesture recognizers)
        gesture/LFGestureRecognizer.cpp
)

if(EMSCRIPTEN)
    message(STATUS "Building for WebAssembly (Emscripten)")

    add_link_options(
            -sUSE_WEBGL2=1
            -sFULL_ES3=1
            -sALLOW_MEMORY_GROWTH=1   # 允许内存动态增长
            -sASSERTIONS=1            # 开启断言调试
            -sFETCH=1
            # -sASYNCIFY=1            # 如果后面需要处理复杂的 QuickJS 异步，可能需要开启
    )

    add_executable(leaf-web
            ${CORE_SOURCES}
            web_main.cpp
    )

    set_target_properties(leaf-web PROPERTIES SUFFIX ".html")

    # 宏定义
    target_compile_definitions(leaf-web PRIVATE
            _GNU_SOURCE
            CONFIG_VERSION="2025-09-13"
            __STDC_FORMAT_MACROS # 关键：解决 PRId64 报错
            CONFIG_BIGNUM
            PLATFORM_WEB # 自定义宏，用于在代码中判断环境
    )

    target_link_options(leaf-web PRIVATE --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/fonts@/fonts)
    target_link_options(leaf-web PRIVATE --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html)

elseif(ANDROID)
    message(STATUS "Building for Android")

    add_library(${CMAKE_PROJECT_NAME} SHARED
            ${CORE_SOURCES}
            leaf_engine_jni.cpp
            engine_bridge.cpp
    )

    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
            _GNU_SOURCE
            CONFIG_VERSION="2025-09-13"
            __STDC_FORMAT_MACROS # 关键：解决 PRId64 报错
            CONFIG_BIGNUM
    )

    target_link_libraries(${CMAKE_PROJECT_NAME}
            GLESv3
            android
            log
            m
    )

else()
    message(WARNING "Building for generic platform (Desktop/Linux?)")
endif()