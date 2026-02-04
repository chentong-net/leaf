//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFBOX_H
#define LEAF_LFBOX_H

#include "LFNode.h"

// 对齐方式
enum class LFBoxAlign {
    TopCenter,
    CenterLeft,
    CenterRight,
    BottomCenter,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Center,      // 居中
    MatchParent  // 撑满
};

/**
 * 层叠布局
 */
class LFBox : public LFNode {
public:
    LFBox();
    virtual ~LFBox() = default;

    /**
     * 添加一个“定位”子节点
     * @param child 子节点
     * @param align 对齐方式
     * @param offsetX X轴偏移 (像素)
     * @param offsetY Y轴偏移 (像素)
     */
    void addChild(const LFNode::Ptr& child, LFBoxAlign align, float offsetX = 0.0f, float offsetY = 0.0f);

    static std::shared_ptr<LFBox> create();
};

#endif // LEAF_LFBOX_H