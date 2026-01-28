cmake_minimum_required(VERSION 3.22.1)

project("leaf-android")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../core)
set(DEPS_DIR ${CORE_DIR}/../third_party)
set(QUICKJS_DIR ${DEPS_DIR}/quickjs)
set(NANOVG_DIR ${DEPS_DIR}/nanovg/src)
set(YOGA_DIR ${DEPS_DIR}/yoga)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CORE_DIR}
        ${QUICKJS_DIR}
        ${NANOVG_DIR}
        ${YOGA_DIR}
)

add_library(${CMAKE_PROJECT_NAME} SHARED
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

        ${CORE_DIR}/LFEngine.cpp
        ${CORE_DIR}/LFBox.cpp
        ${CORE_DIR}/LFLinear.cpp
        ${CORE_DIR}/LFImage.cpp
        ${CORE_DIR}/LFNode.cpp
        ${CORE_DIR}/LFText.cpp
        ${CORE_DIR}/event/LFHitTest.cpp
        ${CORE_DIR}/event/LFEventDispatcher.cpp
        ${CORE_DIR}/event/LFGestureArena.cpp
        ${CORE_DIR}/gesture/LFGestureRecognizer.cpp
        ${CORE_DIR}/animation/LFEasing.cpp
        ${CORE_DIR}/animation/LFEvaluator.cpp
        ${CORE_DIR}/animation/LFSpring.cpp
        ${CORE_DIR}/animation/LFAnimator.cpp
        ${CORE_DIR}/animation/LFAnimationSet.cpp
        ${CORE_DIR}/LFButton.cpp
        ${CORE_DIR}/LFGlobalAnimationManager.cpp
        ${CORE_DIR}/LFScrollView.cpp
        ${CORE_DIR}/LFPage.cpp
        ${CORE_DIR}/LFNavigator.cpp

        leaf_engine_jni.cpp
        engine_bridge.cpp
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        _GNU_SOURCE
        CONFIG_VERSION="2025-09-13"
        __STDC_FORMAT_MACROS  # 关键：解决 PRId64 报错
        CONFIG_BIGNUM
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        GLESv3
        android
        log
        m
)