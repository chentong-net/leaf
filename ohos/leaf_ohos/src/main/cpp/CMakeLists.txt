# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project("leaf-ohos")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../core)
set(DEPS_DIR ${CORE_DIR}/../third_party)
set(QUICKJS_DIR ${DEPS_DIR}/quickjs)
set(NANOVG_DIR ${DEPS_DIR}/nanovg/src)
set(YOGA_DIR ${DEPS_DIR}/yoga)

set(READER_APP_EXAMPLE_DIR ${CORE_DIR}/../examples/reader_app)
set(PROFILE_EXAMPLE_DIR ${CORE_DIR}/../examples/my_profile)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CORE_DIR}
        ${QUICKJS_DIR}
        ${NANOVG_DIR}
        ${YOGA_DIR}
        ${READER_APP_EXAMPLE_DIR}
        ${PROFILE_EXAMPLE_DIR}
)

add_library(leaf_ohos SHARED
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

        ${CORE_DIR}/view/base/LFNode.cpp
        ${CORE_DIR}/view/base/LFText.cpp
        ${CORE_DIR}/view/base/LFImage.cpp
        ${CORE_DIR}/view/base/LFInput.cpp
        ${CORE_DIR}/view/base/LFNavigator.cpp
        ${CORE_DIR}/view/base/LFPage.cpp
        ${CORE_DIR}/view/layout/LFLinear.cpp
        ${CORE_DIR}/view/layout/LFBox.cpp
        ${CORE_DIR}/view/layout/LFGrid.cpp
        ${CORE_DIR}/view/wrapped/LFButton.cpp
        ${CORE_DIR}/view/wrapped/LFScrollView.cpp
        ${CORE_DIR}/view/wrapped/LFTab.cpp
        ${CORE_DIR}/view/wrapped/LFPageView.cpp
        ${CORE_DIR}/view/wrapped/LFListView.cpp
        ${CORE_DIR}/event/LFHitTest.cpp
        ${CORE_DIR}/event/LFEventDispatcher.cpp
        ${CORE_DIR}/event/LFGestureArena.cpp
        ${CORE_DIR}/gesture/LFGestureRecognizer.cpp
        ${CORE_DIR}/animation/LFEasing.cpp
        ${CORE_DIR}/animation/LFEvaluator.cpp
        ${CORE_DIR}/animation/LFSpring.cpp
        ${CORE_DIR}/animation/LFAnimator.cpp
        ${CORE_DIR}/animation/LFAnimationSet.cpp
        ${CORE_DIR}/LFGlobalAnimationManager.cpp
        ${CORE_DIR}/LFEngine.cpp

        ${READER_APP_EXAMPLE_DIR}/ReaderApp.cpp
        ${READER_APP_EXAMPLE_DIR}/BookshelfPage.cpp
        ${READER_APP_EXAMPLE_DIR}/ReaderPage.cpp
        ${READER_APP_EXAMPLE_DIR}/TextSplitter.cpp

        ${PROFILE_EXAMPLE_DIR}/ProfilePage.cpp

        leaf_engine_napi.cpp
        engine_bridge.cpp
)

target_compile_definitions(leaf_ohos PRIVATE
        _GNU_SOURCE
        CONFIG_VERSION="2025-09-13"
        __STDC_FORMAT_MACROS  # 关键：解决 PRId64 报错
        CONFIG_BIGNUM
        __OHOS__
)

target_link_libraries(leaf_ohos PUBLIC
        libace_ndk.z.so
        libace_napi.z.so
        librawfile.z.so
        libGLESv3.so
        libEGL.so
        libhilog_ndk.z.so
)
