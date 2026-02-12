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
    // Box 默认不需要特定的 Flex 方向，通常作为一个 Wrapper
}

void LFBox::addChild(const LFNode::Ptr& child, LFBoxAlign align, float offsetX, float offsetY) {
    LFBoxLayoutParams layoutParams;
    layoutParams.align = align;

    switch (align) {
        case LFBoxAlign::TopLeft:
            layoutParams.margin.start = offsetX;
            layoutParams.margin.top = offsetY;
            break;
        case LFBoxAlign::TopRight:
            layoutParams.margin.end = offsetX;
            layoutParams.margin.top = offsetY;
            break;
        case LFBoxAlign::BottomLeft:
            layoutParams.margin.start = offsetX;
            layoutParams.margin.bottom = offsetY;
            break;
        case LFBoxAlign::BottomRight:
            layoutParams.margin.end = offsetX;
            layoutParams.margin.bottom = offsetY;
            break;
        case LFBoxAlign::MatchParent:
            layoutParams.margin.start = offsetX;
            layoutParams.margin.top = offsetY;
            layoutParams.margin.end = 0.0f;
            layoutParams.margin.bottom = 0.0f;
            break;
        case LFBoxAlign::Center:
            layoutParams.margin.start = offsetX;
            layoutParams.margin.top = offsetY;
            break;
        case LFBoxAlign::TopCenter:
            layoutParams.margin.start = offsetX;
            layoutParams.margin.top = offsetY;
            break;
        case LFBoxAlign::BottomCenter:
            layoutParams.margin.start = offsetX;
            layoutParams.margin.bottom = offsetY;
            break;
        case LFBoxAlign::CenterLeft:
            layoutParams.margin.start = offsetX;
            layoutParams.margin.top = offsetY;
            break;
        case LFBoxAlign::CenterRight:
            layoutParams.margin.end = offsetX;
            layoutParams.margin.top = offsetY;
            break;
    }

    addChild(child, layoutParams);
}

void LFBox::addChild(const LFNode::Ptr& child, const LFBoxLayoutParams& layoutParams) {
    LFNode::addChild(child);
    applyLayoutParams(child, layoutParams);
    m_layoutParams[child.get()] = layoutParams;
}

void LFBox::applyLayoutParams(const LFNode::Ptr& child,
                              const LFBoxLayoutParams& layoutParams) {
    if (!child) return;

    child->setPositionType(YGPositionTypeAbsolute);

    // 清理旧定位，避免重复添加或切换对齐时残留约束
    child->setPosition(YGEdgeLeft, YGUndefined);
    child->setPosition(YGEdgeTop, YGUndefined);
    child->setPosition(YGEdgeRight, YGUndefined);
    child->setPosition(YGEdgeBottom, YGUndefined);

    child->setTranslate(0.0f, 0.0f);
    child->setTranslatePercent(0.0f, 0.0f);

    const LFBoxInsets& margin = layoutParams.margin;
    float shiftX = margin.start - margin.end;
    float shiftY = margin.top - margin.bottom;

    switch (layoutParams.align) {
        case LFBoxAlign::TopLeft:
            child->setPosition(YGEdgeLeft, margin.start);
            child->setPosition(YGEdgeTop, margin.top);
            break;
        case LFBoxAlign::TopRight:
            child->setPosition(YGEdgeRight, margin.end);
            child->setPosition(YGEdgeTop, margin.top);
            break;
        case LFBoxAlign::BottomLeft:
            child->setPosition(YGEdgeLeft, margin.start);
            child->setPosition(YGEdgeBottom, margin.bottom);
            break;
        case LFBoxAlign::BottomRight:
            child->setPosition(YGEdgeRight, margin.end);
            child->setPosition(YGEdgeBottom, margin.bottom);
            break;
        case LFBoxAlign::MatchParent:
            child->setPosition(YGEdgeLeft, margin.start);
            child->setPosition(YGEdgeTop, margin.top);
            child->setPosition(YGEdgeRight, margin.end);
            child->setPosition(YGEdgeBottom, margin.bottom);
            break;
        case LFBoxAlign::Center:
            child->setPositionPercent(YGEdgeLeft, 50.0f);
            child->setPositionPercent(YGEdgeTop, 50.0f);
            child->setTranslatePercent(-50.0f, -50.0f);
            child->setTranslate(shiftX, shiftY);
            break;
        case LFBoxAlign::TopCenter:
            child->setPositionPercent(YGEdgeLeft, 50.0f);
            child->setPosition(YGEdgeTop, margin.top);
            child->setTranslatePercent(-50.0f, 0.0f);
            child->setTranslate(shiftX, 0.0f);
            break;
        case LFBoxAlign::BottomCenter:
            child->setPositionPercent(YGEdgeLeft, 50.0f);
            child->setPosition(YGEdgeBottom, margin.bottom);
            child->setTranslatePercent(-50.0f, 0.0f);
            child->setTranslate(shiftX, 0.0f);
            break;
        case LFBoxAlign::CenterLeft:
            child->setPosition(YGEdgeLeft, margin.start);
            child->setPositionPercent(YGEdgeTop, 50.0f);
            child->setTranslatePercent(0.0f, -50.0f);
            child->setTranslate(0.0f, shiftY);
            break;
        case LFBoxAlign::CenterRight:
            child->setPosition(YGEdgeRight, margin.end);
            child->setPositionPercent(YGEdgeTop, 50.0f);
            child->setTranslatePercent(0.0f, -50.0f);
            child->setTranslate(0.0f, shiftY);
            break;
    }
}

LFBoxLayoutParams LFBox::getLayoutParamsForChild(const LFNode::Ptr& child) const {
    if (!child) {
        return {};
    }
    auto it = m_layoutParams.find(child.get());
    if (it != m_layoutParams.end()) {
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
    return isWrapContentStyle(YGNodeStyleGetWidth(getYGNode()));
}

bool LFBox::isWrapContentHeight() const {
    return isWrapContentStyle(YGNodeStyleGetHeight(getYGNode()));
}

void LFBox::onBeforeCalculateLayout(float ownerWidth, float ownerHeight) {
    for (auto it = m_layoutParams.begin(); it != m_layoutParams.end();) {
        bool exists = false;
        for (const auto& child : getChildren()) {
            if (child && child.get() == it->first) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            it = m_layoutParams.erase(it);
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

        YGValue childWidthStyle = YGNodeStyleGetWidth(childNode);
        YGValue childHeightStyle = YGNodeStyleGetHeight(childNode);
        float childOwnerWidth = resolveChildMeasureSize(childNode, childWidthStyle, contentOwnerWidth);
        float childOwnerHeight = resolveChildMeasureSize(childNode, childHeightStyle, contentOwnerHeight);

        if (YGFloatIsUndefined(childOwnerWidth)) {
            childOwnerWidth = contentOwnerWidth;
        }
        if (YGFloatIsUndefined(childOwnerHeight)) {
            childOwnerHeight = contentOwnerHeight;
        }

        YGNodeCalculateLayout(childNode, childOwnerWidth, childOwnerHeight, YGDirectionLTR);

        float childWidth = YGNodeLayoutGetWidth(childNode);
        float childHeight = YGNodeLayoutGetHeight(childNode);

        LFBoxLayoutParams params = getLayoutParamsForChild(child);
        float marginStart = params.margin.start + resolveMargin(childNode, YGEdgeLeft, contentOwnerWidth);
        float marginTop = params.margin.top + resolveMargin(childNode, YGEdgeTop, contentOwnerHeight);
        float marginEnd = params.margin.end + resolveMargin(childNode, YGEdgeRight, contentOwnerWidth);
        float marginBottom = params.margin.bottom + resolveMargin(childNode, YGEdgeBottom, contentOwnerHeight);

        bool hasLeft = isValueDefined(YGNodeStyleGetPosition(childNode, YGEdgeLeft));
        bool hasRight = isValueDefined(YGNodeStyleGetPosition(childNode, YGEdgeRight));
        bool hasTop = isValueDefined(YGNodeStyleGetPosition(childNode, YGEdgeTop));
        bool hasBottom = isValueDefined(YGNodeStyleGetPosition(childNode, YGEdgeBottom));
        bool widthPercent100 = childWidthStyle.unit == YGUnitPercent && almostEqual(childWidthStyle.value, 100.0f);
        bool heightPercent100 = childHeightStyle.unit == YGUnitPercent && almostEqual(childHeightStyle.value, 100.0f);
        bool matchParentWidth = params.align == LFBoxAlign::MatchParent || widthPercent100 || (hasLeft && hasRight);
        bool matchParentHeight = params.align == LFBoxAlign::MatchParent || heightPercent100 || (hasTop && hasBottom);

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

std::shared_ptr<LFBox> LFBox::create() {
    return std::make_shared<LFBox>();
}
