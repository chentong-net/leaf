//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFBOX_H
#define LEAF_LFBOX_H

#include "view/base/LFNode.h"

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

struct LFBoxInsets {
    float start = 0.0f;
    float top = 0.0f;
    float end = 0.0f;
    float bottom = 0.0f;
};

struct LFBoxLayoutParams {
    LFBoxAlign align = LFBoxAlign::TopLeft;
    LFBoxInsets margin;
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
    void addChild(const LFNode::Ptr& child, const LFBoxLayoutParams& layoutParams);

    static std::shared_ptr<LFBox> create();

protected:
    void onBeforeCalculateLayout(float ownerWidth, float ownerHeight) override;
    void onAfterCalculateLayout() override;

private:
    void applyLayoutParams(const LFNode::Ptr& child, const LFBoxLayoutParams& layoutParams);
    LFBoxLayoutParams getLayoutParamsForChild(const LFNode::Ptr& child) const;
    float resolveChildMeasureSize(YGNodeRef node, YGValue value, float ownerSize) const;
    float resolveMargin(YGNodeRef node, YGEdge edge, float ownerSize) const;
    float resolvePadding(YGEdge edge, float ownerSize) const;
    float resolveBorder(YGEdge edge) const;
    bool isWrapContentWidth() const;
    bool isWrapContentHeight() const;

    std::unordered_map<LFNode*, LFBoxLayoutParams> m_layoutParams;
    bool m_restoreAutoWidth = false;
    bool m_restoreAutoHeight = false;
};

#endif // LEAF_LFBOX_H
