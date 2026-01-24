cmake_minimum_required(VERSION 3.22.1)

project("leaf-android")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)
set(QUICKJS_DIR ${DEPS_DIR}/quickjs)
set(NANOVG_DIR ${DEPS_DIR}/nanovg/src)
set(YOGA_DIR ${DEPS_DIR}/yoga)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${QUICKJS_DIR}
        ${NANOVG_DIR}
        ${YOGA_DIR}
)

add_library(${CMAKE_PROJECT_NAME} SHARED
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
        event/LFHitTest.cpp
        event/LFEventDispatcher.cpp
        event/LFGestureArena.cpp
        gesture/LFGestureRecognizer.cpp
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
        log
        m
)