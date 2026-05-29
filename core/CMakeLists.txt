cmake_minimum_required(VERSION 3.22.1)

project("leaf-core")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)
set(NANOVG_DIR ${DEPS_DIR}/nanovg/src)
set(YOGA_DIR ${DEPS_DIR}/yoga)
set(GLAD_DIR ${DEPS_DIR}/glad)

set(LEAF_CORE_SOURCES
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

        ${CMAKE_CURRENT_SOURCE_DIR}/view/base/LFNode.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/base/LFText.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/base/LFImage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/base/LFTexture.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/base/LFInput.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/base/LFNavigator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/base/LFPage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/layout/LFLinear.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/layout/LFBox.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/layout/LFGrid.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/wrapped/LFButton.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/wrapped/LFScrollView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/wrapped/LFTab.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/wrapped/LFPageView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/wrapped/LFListView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/wrapped/LFDropdown.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/view/wrapped/LFToggle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/event/LFHitTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/event/LFEventDispatcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/event/LFGestureArena.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/plugin/LFPlugin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gesture/LFGestureRecognizer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/animation/LFEasing.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/animation/LFEvaluator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/animation/LFSpring.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/animation/LFAnimator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/animation/LFAnimationSet.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/plugin/LFNativeSender.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/LFGlobalAnimationManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/LFEngine.cpp
)

if(LEAF_PLATFORM_DEF STREQUAL "DESKTOP")
    list(APPEND LEAF_CORE_SOURCES
            ${GLAD_DIR}/glad.c
    )
endif()

add_library(leaf-core ${LEAF_CORE_SOURCES})
set_target_properties(leaf-core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(leaf-core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NANOVG_DIR}
        ${YOGA_DIR}
)

if (LEAF_PLATFORM_DEF STREQUAL "DESKTOP")
    target_include_directories(leaf-core PUBLIC
            ${GLAD_DIR}
    )
endif()
