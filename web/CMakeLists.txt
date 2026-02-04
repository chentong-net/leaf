cmake_minimum_required(VERSION 3.22.1)

project("leaf-web")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core)
set(DEPS_DIR ${CORE_DIR}/../third_party)
set(QUICKJS_DIR ${DEPS_DIR}/quickjs)
set(NANOVG_DIR ${DEPS_DIR}/nanovg/src)
set(YOGA_DIR ${DEPS_DIR}/yoga)

set(READER_APP_EXAMPLE_DIR ${CORE_DIR}/../examples/reader_app)
set(PROFILE_EXAMPLE_DIR ${CORE_DIR}/../examples/my_profile)

include_directories(
        ${CORE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${QUICKJS_DIR}
        ${NANOVG_DIR}
        ${YOGA_DIR}
        ${READER_APP_EXAMPLE_DIR}
        ${PROFILE_EXAMPLE_DIR}
)

set(CORE_SOURCES
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

        ${CORE_DIR}/component/LFBox.cpp
        ${CORE_DIR}/component/LFLinear.cpp
        ${CORE_DIR}/component/LFImage.cpp
        ${CORE_DIR}/component/LFNode.cpp
        ${CORE_DIR}/component/LFText.cpp
        ${CORE_DIR}/event/LFHitTest.cpp
        ${CORE_DIR}/event/LFEventDispatcher.cpp
        ${CORE_DIR}/event/LFGestureArena.cpp
        ${CORE_DIR}/gesture/LFGestureRecognizer.cpp
        ${CORE_DIR}/animation/LFEasing.cpp
        ${CORE_DIR}/animation/LFEvaluator.cpp
        ${CORE_DIR}/animation/LFSpring.cpp
        ${CORE_DIR}/animation/LFAnimator.cpp
        ${CORE_DIR}/animation/LFAnimationSet.cpp
        ${CORE_DIR}/component/LFButton.cpp
        ${CORE_DIR}/component/LFScrollView.cpp
        ${CORE_DIR}/component/LFPage.cpp
        ${CORE_DIR}/component/LFNavigator.cpp
        ${CORE_DIR}/component/LFGrid.cpp
        ${CORE_DIR}/component/LFTab.cpp
        ${CORE_DIR}/component/LFPageView.cpp
        ${CORE_DIR}/LFGlobalAnimationManager.cpp
        ${CORE_DIR}/LFEngine.cpp
)

message(STATUS "Building for WebAssembly")

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
        ${READER_APP_EXAMPLE_DIR}/ReaderApp.cpp
        ${READER_APP_EXAMPLE_DIR}/BookshelfPage.cpp
        ${READER_APP_EXAMPLE_DIR}/ReaderPage.cpp
        ${READER_APP_EXAMPLE_DIR}/TextSplitter.cpp
        ${PROFILE_EXAMPLE_DIR}/ProfilePage.cpp
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