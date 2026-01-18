//
// Created by Chen Tong on 2026/1/18.
//

#include "LFBox.h"

LFBox::LFBox() {
    // Box 默认不需要特定的 Flex 方向，通常作为一个 Wrapper
}

void LFBox::addChild(const LFNode::Ptr& child, LFBoxAlign align, float offsetX, float offsetY) {
    // 1. 添加到树
    LFNode::addChild(child);

    // 2. 开启绝对定位
    child->setPositionType(YGPositionTypeAbsolute);

    // 3. 重置所有边距约束 (防止旧约束干扰)
    // 注意：Yoga 中 NAN 代表 Undefined
    child->setPosition(YGEdgeLeft, NAN);
    child->setPosition(YGEdgeTop, NAN);
    child->setPosition(YGEdgeRight, NAN);
    child->setPosition(YGEdgeBottom, NAN);

    // 必须重置 Transform，否则之前的 Center 设置可能会残留
    child->setTranslatePercent(0, 0);

    // 4. 应用对齐策略
    switch (align) {
        case LFBoxAlign::TopLeft:
            child->setPosition(YGEdgeLeft, offsetX);
            child->setPosition(YGEdgeTop, offsetY);
            break;

        case LFBoxAlign::TopRight:
            child->setPosition(YGEdgeRight, offsetX);
            child->setPosition(YGEdgeTop, offsetY);
            break;

        case LFBoxAlign::BottomLeft:
            child->setPosition(YGEdgeLeft, offsetX);
            child->setPosition(YGEdgeBottom, offsetY);
            break;

        case LFBoxAlign::BottomRight:
            child->setPosition(YGEdgeRight, offsetX);
            child->setPosition(YGEdgeBottom, offsetY);
            break;

        case LFBoxAlign::MatchParent:
            child->setPosition(YGEdgeAll, 0); // 上下左右全为0 -> 撑满
            break;

        case LFBoxAlign::Center:
            // 实现绝对定位居中
            // 1. 让左上角跑到父容器中心 (Yoga 布局阶段)
            child->setPositionPercent(YGEdgeLeft, 50.0f);
            child->setPositionPercent(YGEdgeTop, 50.0f);

            // 2. 往回移动自身宽高的一半 (渲染阶段)
            // 这需要 LFNode 支持 setTranslatePercent
            child->setTranslatePercent(-50.0f, -50.0f);

            // 应用额外的偏移
            if (offsetX != 0 || offsetY != 0) {
                child->setTranslate(offsetX, offsetY);
            }
            break;
    }
}

std::shared_ptr<LFBox> LFBox::create() {
    return std::make_shared<LFBox>();
}