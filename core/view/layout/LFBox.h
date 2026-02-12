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

/**
 * 层叠布局
 */
class LFBox : public LFNode {
public:
    using Ptr = std::shared_ptr<LFBox>;
    using LFNode::addChild;

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

    // 覆盖同名接口，仅用于记录LFBox自身的wrap_content意图
    void setWidth(float width);
    void setHeight(float height);
    void setWidthPercent(float percent);
    void setHeightPercent(float percent);
    void matchParentWidth();
    void matchParentHeight();
    void wrapContentWidth();
    void wrapContentHeight();

    static Ptr create();

protected:
    void onBeforeCalculateLayout(float ownerWidth, float ownerHeight) override;
    void onAfterCalculateLayout() override;

private:
    struct ChildLayoutMeta {
        LFBoxAlign align = LFBoxAlign::TopLeft;
        float marginStart = 0.0f;
        float marginTop = 0.0f;
        float marginEnd = 0.0f;
        float marginBottom = 0.0f;
    };

    void applyLayoutMeta(const LFNode::Ptr& child, const ChildLayoutMeta& meta);
    ChildLayoutMeta getLayoutMetaForChild(const LFNode::Ptr& child) const;
    float resolveChildMeasureSize(YGNodeRef node, YGValue value, float ownerSize) const;
    float resolveMargin(YGNodeRef node, YGEdge edge, float ownerSize) const;
    float resolvePadding(YGEdge edge, float ownerSize) const;
    float resolveBorder(YGEdge edge) const;
    bool isWrapContentWidth() const;
    bool isWrapContentHeight() const;

    std::unordered_map<LFNode*, ChildLayoutMeta> m_layoutMeta;
    bool m_restoreAutoWidth = false;
    bool m_restoreAutoHeight = false;
    bool m_wrapContentWidthRequested = false;
    bool m_wrapContentHeightRequested = false;
};

#endif // LEAF_LFBOX_H
