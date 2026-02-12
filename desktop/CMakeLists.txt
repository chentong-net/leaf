cmake_minimum_required(VERSION 3.22.1)

project("leaf-desktop")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core)
set(DEPS_DIR ${CORE_DIR}/../third_party)
set(QUICKJS_DIR ${DEPS_DIR}/quickjs)
set(NANOVG_DIR ${DEPS_DIR}/nanovg/src)
set(YOGA_DIR ${DEPS_DIR}/yoga)
set(GLAD_DIR ${DEPS_DIR}/glad)
set(GLFW_DIR ${DEPS_DIR}/glfw)

set(READER_APP_EXAMPLE_DIR ${CORE_DIR}/../examples/reader_app)
set(PROFILE_EXAMPLE_DIR ${CORE_DIR}/../examples/my_profile)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_DIR} glfw_build)

include_directories(
        ${CORE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${QUICKJS_DIR}
        ${NANOVG_DIR}
        ${YOGA_DIR}
        ${GLAD_DIR}
        ${READER_APP_EXAMPLE_DIR}
        ${PROFILE_EXAMPLE_DIR}
        ${GLFW_DIR}/include
)

set(CORE_SOURCES
        ${QUICKJS_DIR}/quickjs.c
        ${QUICKJS_DIR}/libregexp.c
        ${QUICKJS_DIR}/libunicode.c
        ${QUICKJS_DIR}/cutils.c
        ${QUICKJS_DIR}/dtoa.c

        ${GLAD_DIR}/glad.c

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
)

message(STATUS "Building for Desktop")

add_executable(leaf-desktop
        ${CORE_SOURCES}
        ${READER_APP_EXAMPLE_DIR}/ReaderApp.cpp
        ${READER_APP_EXAMPLE_DIR}/BookshelfPage.cpp
        ${READER_APP_EXAMPLE_DIR}/ReaderPage.cpp
        ${READER_APP_EXAMPLE_DIR}/TextSplitter.cpp
        ${PROFILE_EXAMPLE_DIR}/ProfilePage.cpp
        desktop_main.cpp
)

#target_include_directories(leaf-desktop PRIVATE ${GLFW_DIR}/include)
target_link_libraries(leaf-desktop PRIVATE glfw)

#if(APPLE)
#    target_link_libraries(leaf-desktop PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
#endif()

# 宏定义
target_compile_definitions(leaf-desktop PRIVATE
        _GNU_SOURCE
        CONFIG_VERSION="2025-09-13"
        __STDC_FORMAT_MACROS # 关键：解决 PRId64 报错
        CONFIG_BIGNUM
        __DESKTOP__
)
