//
// Created by Chen Tong on 2026/1/18.
//

#include "view/layout/LFBox.h"

#include <cmath>

namespace {

bool isWrapContentStyle(const YGValue& value) {
    return value.unit == YGUnitAuto;
}

bool isValueDefined(const YGValue& value) {
    return value.unit != YGUnitUndefined;
}

float resolveValue(const YGValue& value, float ownerSize) {
    if (value.unit == YGUnitPoint) {
        return value.value;
    }
    if (value.unit == YGUnitPercent && !YGFloatIsUndefined(ownerSize)) {
        return ownerSize * value.value / 100.0f;
    }
    return 0.0f;
}

YGValue resolveEdgeValue(YGNodeRef node, YGEdge edge, YGValue (*getter)(YGNodeConstRef, YGEdge)) {
    YGValue value = getter(node, edge);
    if (value.unit != YGUnitUndefined) {
        return value;
    }

    if (edge == YGEdgeLeft || edge == YGEdgeRight) {
        value = getter(node, YGEdgeHorizontal);
        if (value.unit != YGUnitUndefined) {
            return value;
        }
    } else if (edge == YGEdgeTop || edge == YGEdgeBottom) {
        value = getter(node, YGEdgeVertical);
        if (value.unit != YGUnitUndefined) {
            return value;
        }
    }

    return getter(node, YGEdgeAll);
}

float resolveBorderValue(YGNodeRef node, YGEdge edge) {
    float value = YGNodeStyleGetBorder(node, edge);
    if (!YGFloatIsUndefined(value)) {
        return value;
    }

    if (edge == YGEdgeLeft || edge == YGEdgeRight) {
        value = YGNodeStyleGetBorder(node, YGEdgeHorizontal);
        if (!YGFloatIsUndefined(value)) {
            return value;
        }
    } else if (edge == YGEdgeTop || edge == YGEdgeBottom) {
        value = YGNodeStyleGetBorder(node, YGEdgeVertical);
        if (!YGFloatIsUndefined(value)) {
            return value;
        }
    }

    value = YGNodeStyleGetBorder(node, YGEdgeAll);
    if (!YGFloatIsUndefined(value)) {
        return value;
    }
    return 0.0f;
}

bool almostEqual(float a, float b) {
    return std::fabs(a - b) <= 0.001f;
}

}

LFBox::LFBox() {
    // Box默认不需要特定的Flex方向，通常作为一个Wrapper
}

void LFBox::setWidth(float width) {
    m_wrapContentWidthRequested = width < 0.0f;
    LFNode::setWidth(width);
}

void LFBox::setHeight(float height) {
    m_wrapContentHeightRequested = height < 0.0f;
    LFNode::setHeight(height);
}

void LFBox::setWidthPercent(float percent) {
    m_wrapContentWidthRequested = false;
    LFNode::setWidthPercent(percent);
}

void LFBox::setHeightPercent(float percent) {
    m_wrapContentHeightRequested = false;
    LFNode::setHeightPercent(percent);
}

void LFBox::matchParentWidth() {
    m_wrapContentWidthRequested = false;
    LFNode::matchParentWidth();
}

void LFBox::matchParentHeight() {
    m_wrapContentHeightRequested = false;
    LFNode::matchParentHeight();
}

void LFBox::wrapContentWidth() {
    m_wrapContentWidthRequested = true;
    LFNode::wrapContentWidth();
}

void LFBox::wrapContentHeight() {
    m_wrapContentHeightRequested = true;
    LFNode::wrapContentHeight();
}

void LFBox::addChild(const LFNode::Ptr& child, LFBoxAlign align, float offsetX, float offsetY) {
    if (!child) return;

    ChildLayoutMeta meta;
    meta.align = align;

    switch (align) {
        case LFBoxAlign::TopLeft:
            meta.marginStart = offsetX;
            meta.marginTop = offsetY;
            break;
        case LFBoxAlign::TopRight:
            meta.marginEnd = offsetX;
            meta.marginTop = offsetY;
            break;
        case LFBoxAlign::BottomLeft:
            meta.marginStart = offsetX;
            meta.marginBottom = offsetY;
            break;
        case LFBoxAlign::BottomRight:
            meta.marginEnd = offsetX;
            meta.marginBottom = offsetY;
            break;
        case LFBoxAlign::MatchParent:
            // 兼容旧语义：MatchParent始终四边为0，offset参数忽略
            break;
        case LFBoxAlign::Center:
        case LFBoxAlign::TopCenter:
        case LFBoxAlign::BottomCenter:
        case LFBoxAlign::CenterLeft:
        case LFBoxAlign::CenterRight:
            // 兼容旧语义：中心系对齐的offset通过Translate处理
            break;
    }

    LFNode::addChild(child);
    applyLayoutMeta(child, meta);
    m_layoutMeta[child.get()] = meta;

    switch (align) {
        case LFBoxAlign::Center:
        case LFBoxAlign::TopCenter:
        case LFBoxAlign::BottomCenter:
        case LFBoxAlign::CenterLeft:
        case LFBoxAlign::CenterRight:
            if (offsetX != 0.0f || offsetY != 0.0f) {
                child->setTranslate(offsetX, offsetY);
            }
            break;
        default:
            break;
    }
}

void LFBox::applyLayoutMeta(const LFNode::Ptr& child, const ChildLayoutMeta& meta) {
    if (!child) return;

    child->setPositionType(YGPositionTypeAbsolute);

    child->setPosition(YGEdgeLeft, YGUndefined);
    child->setPosition(YGEdgeTop, YGUndefined);
    child->setPosition(YGEdgeRight, YGUndefined);
    child->setPosition(YGEdgeBottom, YGUndefined);

    child->setTranslate(0.0f, 0.0f);
    child->setTranslatePercent(0.0f, 0.0f);

    switch (meta.align) {
        case LFBoxAlign::TopLeft:
            child->setPosition(YGEdgeLeft, meta.marginStart);
            child->setPosition(YGEdgeTop, meta.marginTop);
            break;
        case LFBoxAlign::TopRight:
            child->setPosition(YGEdgeRight, meta.marginEnd);
            child->setPosition(YGEdgeTop, meta.marginTop);
            break;
        case LFBoxAlign::BottomLeft:
            child->setPosition(YGEdgeLeft, meta.marginStart);
            child->setPosition(YGEdgeBottom, meta.marginBottom);
            break;
        case LFBoxAlign::BottomRight:
            child->setPosition(YGEdgeRight, meta.marginEnd);
            child->setPosition(YGEdgeBottom, meta.marginBottom);
            break;
        case LFBoxAlign::MatchParent:
            child->setPosition(YGEdgeAll, 0.0f);
            break;
        case LFBoxAlign::Center:
            child->setPositionPercent(YGEdgeLeft, 50.0f);
            child->setPositionPercent(YGEdgeTop, 50.0f);
            child->setTranslatePercent(-50.0f, -50.0f);
            break;
        case LFBoxAlign::TopCenter:
            child->setPositionPercent(YGEdgeLeft, 50.0f);
            child->setPosition(YGEdgeTop, 0.0f);
            child->setTranslatePercent(-50.0f, 0.0f);
            break;
        case LFBoxAlign::BottomCenter:
            child->setPositionPercent(YGEdgeLeft, 50.0f);
            child->setPosition(YGEdgeBottom, 0.0f);
            child->setTranslatePercent(-50.0f, 0.0f);
            break;
        case LFBoxAlign::CenterLeft:
            child->setPosition(YGEdgeLeft, 0.0f);
            child->setPositionPercent(YGEdgeTop, 50.0f);
            child->setTranslatePercent(0.0f, -50.0f);
            break;
        case LFBoxAlign::CenterRight:
            child->setPosition(YGEdgeRight, 0.0f);
            child->setPositionPercent(YGEdgeTop, 50.0f);
            child->setTranslatePercent(0.0f, -50.0f);
            break;
    }
}

LFBox::ChildLayoutMeta LFBox::getLayoutMetaForChild(const LFNode::Ptr& child) const {
    if (!child) {
        return {};
    }
    auto it = m_layoutMeta.find(child.get());
    if (it != m_layoutMeta.end()) {
        return it->second;
    }
    return {};
}

float LFBox::resolveChildMeasureSize(YGNodeRef node, YGValue value, float ownerSize) const {
    if (value.unit == YGUnitPoint) {
        return value.value;
    }
    if (value.unit == YGUnitPercent && !YGFloatIsUndefined(ownerSize)) {
        return ownerSize * value.value / 100.0f;
    }
    return YGUndefined;
}

float LFBox::resolveMargin(YGNodeRef node, YGEdge edge, float ownerSize) const {
    return resolveValue(resolveEdgeValue(node, edge, YGNodeStyleGetMargin), ownerSize);
}

float LFBox::resolvePadding(YGEdge edge, float ownerSize) const {
    return resolveValue(resolveEdgeValue(getYGNode(), edge, YGNodeStyleGetPadding), ownerSize);
}

float LFBox::resolveBorder(YGEdge edge) const {
    return resolveBorderValue(getYGNode(), edge);
}

bool LFBox::isWrapContentWidth() const {
    return m_wrapContentWidthRequested && isWrapContentStyle(YGNodeStyleGetWidth(getYGNode()));
}

bool LFBox::isWrapContentHeight() const {
    return m_wrapContentHeightRequested && isWrapContentStyle(YGNodeStyleGetHeight(getYGNode()));
}

void LFBox::onBeforeCalculateLayout(float ownerWidth, float ownerHeight) {
    for (auto it = m_layoutMeta.begin(); it != m_layoutMeta.end();) {
        bool exists = false;
        for (const auto& child : getChildren()) {
            if (child && child.get() == it->first) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            it = m_layoutMeta.erase(it);
        } else {
            ++it;
        }
    }

    m_restoreAutoWidth = false;
    m_restoreAutoHeight = false;

    bool wrapWidth = isWrapContentWidth();
    bool wrapHeight = isWrapContentHeight();
    if (!wrapWidth && !wrapHeight) {
        return;
    }

    float borderLeft = resolveBorder(YGEdgeLeft);
    float borderTop = resolveBorder(YGEdgeTop);
    float borderRight = resolveBorder(YGEdgeRight);
    float borderBottom = resolveBorder(YGEdgeBottom);
    float paddingLeft = resolvePadding(YGEdgeLeft, ownerWidth);
    float paddingTop = resolvePadding(YGEdgeTop, ownerHeight);
    float paddingRight = resolvePadding(YGEdgeRight, ownerWidth);
    float paddingBottom = resolvePadding(YGEdgeBottom, ownerHeight);

    float contentOwnerWidth = ownerWidth;
    float contentOwnerHeight = ownerHeight;
    if (!YGFloatIsUndefined(contentOwnerWidth)) {
        contentOwnerWidth = std::max(0.0f, contentOwnerWidth - borderLeft - borderRight - paddingLeft - paddingRight);
    }
    if (!YGFloatIsUndefined(contentOwnerHeight)) {
        contentOwnerHeight = std::max(0.0f, contentOwnerHeight - borderTop - borderBottom - paddingTop - paddingBottom);
    }

    float measuredChildrenWidth = 0.0f;
    float measuredChildrenHeight = 0.0f;

    for (const auto& child : getChildren()) {
        if (!child) continue;

        YGNodeRef childNode = child->getYGNode();
        if (YGNodeStyleGetDisplay(childNode) == YGDisplayNone) {
            continue;
        }

        ChildLayoutMeta meta = getLayoutMetaForChild(child);
        YGValue childWidthStyle = YGNodeStyleGetWidth(childNode);
        YGValue childHeightStyle = YGNodeStyleGetHeight(childNode);

        bool hasLeft = isValueDefined(YGNodeStyleGetPosition(childNode, YGEdgeLeft));
        bool hasRight = isValueDefined(YGNodeStyleGetPosition(childNode, YGEdgeRight));
        bool hasTop = isValueDefined(YGNodeStyleGetPosition(childNode, YGEdgeTop));
        bool hasBottom = isValueDefined(YGNodeStyleGetPosition(childNode, YGEdgeBottom));
        bool widthPercent100 = childWidthStyle.unit == YGUnitPercent && almostEqual(childWidthStyle.value, 100.0f);
        bool heightPercent100 = childHeightStyle.unit == YGUnitPercent && almostEqual(childHeightStyle.value, 100.0f);
        bool matchParentWidth = meta.align == LFBoxAlign::MatchParent || widthPercent100 || (hasLeft && hasRight);
        bool matchParentHeight = meta.align == LFBoxAlign::MatchParent || heightPercent100 || (hasTop && hasBottom);

        float childOwnerWidth = resolveChildMeasureSize(childNode, childWidthStyle, contentOwnerWidth);
        float childOwnerHeight = resolveChildMeasureSize(childNode, childHeightStyle, contentOwnerHeight);

        // 关键策略：wrap轴上auto子节点应按自然尺寸测量，不应强制吃满父尺寸
        if (YGFloatIsUndefined(childOwnerWidth)) {
            childOwnerWidth = (wrapWidth && !matchParentWidth) ? YGUndefined : contentOwnerWidth;
        }
        if (YGFloatIsUndefined(childOwnerHeight)) {
            childOwnerHeight = (wrapHeight && !matchParentHeight) ? YGUndefined : contentOwnerHeight;
        }

        YGNodeCalculateLayout(childNode, childOwnerWidth, childOwnerHeight, YGDirectionLTR);

        float childWidth = YGNodeLayoutGetWidth(childNode);
        float childHeight = YGNodeLayoutGetHeight(childNode);

        float marginStart = meta.marginStart + resolveMargin(childNode, YGEdgeLeft, contentOwnerWidth);
        float marginTop = meta.marginTop + resolveMargin(childNode, YGEdgeTop, contentOwnerHeight);
        float marginEnd = meta.marginEnd + resolveMargin(childNode, YGEdgeRight, contentOwnerWidth);
        float marginBottom = meta.marginBottom + resolveMargin(childNode, YGEdgeBottom, contentOwnerHeight);

        if (!wrapWidth || !matchParentWidth) {
            measuredChildrenWidth = std::max(measuredChildrenWidth, childWidth + marginStart + marginEnd);
        }
        if (!wrapHeight || !matchParentHeight) {
            measuredChildrenHeight = std::max(measuredChildrenHeight, childHeight + marginTop + marginBottom);
        }
    }

    float measuredWidth = measuredChildrenWidth + paddingLeft + paddingRight + borderLeft + borderRight;
    float measuredHeight = measuredChildrenHeight + paddingTop + paddingBottom + borderTop + borderBottom;

    if (wrapWidth) {
        YGNodeStyleSetWidth(getYGNode(), measuredWidth);
        m_restoreAutoWidth = true;
    }
    if (wrapHeight) {
        YGNodeStyleSetHeight(getYGNode(), measuredHeight);
        m_restoreAutoHeight = true;
    }
}

void LFBox::onAfterCalculateLayout() {
    if (m_restoreAutoWidth) {
        YGNodeStyleSetWidthAuto(getYGNode());
        m_restoreAutoWidth = false;
    }
    if (m_restoreAutoHeight) {
        YGNodeStyleSetHeightAuto(getYGNode());
        m_restoreAutoHeight = false;
    }
}

LFBox::Ptr LFBox::create() {
    return std::make_shared<LFBox>();
}
