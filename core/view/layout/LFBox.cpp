//
// Created by Chen Tong on 2026/1/18.
//

#include "view/layout/LFBox.h"

#include <cmath>
#include <limits>

namespace {

bool isWrapContentStyle(const LFLayoutValue& value) {
    return value.unit == LFLayoutUnit::Auto;
}

bool isValueDefined(const LFLayoutValue& value) {
    return LFLayoutValueIsDefined(value);
}

float resolveValue(const LFLayoutValue& value, float ownerSize) {
    return LFLayoutResolveValue(value, ownerSize);
}

LFLayoutValue resolveEdgeValue(
        const std::array<LFLayoutValue, kLFLayoutEdgeCount>& values,
        LFLayoutEdge edge) {
    return LFLayoutResolveEdgeValue(values, edge);
}

float resolveBorderValue(
        const std::array<LFLayoutValue, kLFLayoutEdgeCount>& values,
        LFLayoutEdge edge) {
    const LFLayoutValue value = resolveEdgeValue(values, edge);
    return value.unit == LFLayoutUnit::Undefined ? 0.0f : value.value;
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

    child->setPositionType(LFPositionType::Absolute);

    const float undefined = std::numeric_limits<float>::quiet_NaN();
    child->setPosition(LFLayoutEdge::Left, undefined);
    child->setPosition(LFLayoutEdge::Top, undefined);
    child->setPosition(LFLayoutEdge::Right, undefined);
    child->setPosition(LFLayoutEdge::Bottom, undefined);

    child->setTranslate(0.0f, 0.0f);
    child->setTranslatePercent(0.0f, 0.0f);

    switch (meta.align) {
        case LFBoxAlign::TopLeft:
            child->setPosition(LFLayoutEdge::Left, meta.marginStart);
            child->setPosition(LFLayoutEdge::Top, meta.marginTop);
            break;
        case LFBoxAlign::TopRight:
            child->setPosition(LFLayoutEdge::Right, meta.marginEnd);
            child->setPosition(LFLayoutEdge::Top, meta.marginTop);
            break;
        case LFBoxAlign::BottomLeft:
            child->setPosition(LFLayoutEdge::Left, meta.marginStart);
            child->setPosition(LFLayoutEdge::Bottom, meta.marginBottom);
            break;
        case LFBoxAlign::BottomRight:
            child->setPosition(LFLayoutEdge::Right, meta.marginEnd);
            child->setPosition(LFLayoutEdge::Bottom, meta.marginBottom);
            break;
        case LFBoxAlign::MatchParent:
            child->setPosition(LFLayoutEdge::All, 0.0f);
            break;
        case LFBoxAlign::Center:
            child->setPositionPercent(LFLayoutEdge::Left, 50.0f);
            child->setPositionPercent(LFLayoutEdge::Top, 50.0f);
            child->setTranslatePercent(-50.0f, -50.0f);
            break;
        case LFBoxAlign::TopCenter:
            child->setPositionPercent(LFLayoutEdge::Left, 50.0f);
            child->setPosition(LFLayoutEdge::Top, 0.0f);
            child->setTranslatePercent(-50.0f, 0.0f);
            break;
        case LFBoxAlign::BottomCenter:
            child->setPositionPercent(LFLayoutEdge::Left, 50.0f);
            child->setPosition(LFLayoutEdge::Bottom, 0.0f);
            child->setTranslatePercent(-50.0f, 0.0f);
            break;
        case LFBoxAlign::CenterLeft:
            child->setPosition(LFLayoutEdge::Left, 0.0f);
            child->setPositionPercent(LFLayoutEdge::Top, 50.0f);
            child->setTranslatePercent(0.0f, -50.0f);
            break;
        case LFBoxAlign::CenterRight:
            child->setPosition(LFLayoutEdge::Right, 0.0f);
            child->setPositionPercent(LFLayoutEdge::Top, 50.0f);
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

float LFBox::resolveChildMeasureSize(const LFLayoutValue& value, float ownerSize) const {
    if (value.unit == LFLayoutUnit::Point) {
        return value.value;
    }
    if (value.unit == LFLayoutUnit::Percent && std::isfinite(ownerSize)) {
        return ownerSize * value.value / 100.0f;
    }
    return std::numeric_limits<float>::quiet_NaN();
}

float LFBox::resolveMargin(const LFNode::Ptr& node,
                           LFLayoutEdge edge,
                           float ownerSize) const {
    if (!node) return 0.0f;
    return resolveValue(
        resolveEdgeValue(node->getLayoutStyle().margin, edge),
        ownerSize
    );
}

float LFBox::resolvePadding(LFLayoutEdge edge, float ownerSize) const {
    return resolveValue(
        resolveEdgeValue(getLayoutStyle().padding, edge),
        ownerSize
    );
}

float LFBox::resolveBorder(LFLayoutEdge edge) const {
    return resolveBorderValue(getLayoutStyle().border, edge);
}

bool LFBox::isWrapContentWidth() const {
    return m_wrapContentWidthRequested && isWrapContentStyle(getLayoutStyle().width);
}

bool LFBox::isWrapContentHeight() const {
    return m_wrapContentHeightRequested && isWrapContentStyle(getLayoutStyle().height);
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

    float borderLeft = resolveBorder(LFLayoutEdge::Left);
    float borderTop = resolveBorder(LFLayoutEdge::Top);
    float borderRight = resolveBorder(LFLayoutEdge::Right);
    float borderBottom = resolveBorder(LFLayoutEdge::Bottom);
    float paddingLeft = resolvePadding(LFLayoutEdge::Left, ownerWidth);
    float paddingTop = resolvePadding(LFLayoutEdge::Top, ownerHeight);
    float paddingRight = resolvePadding(LFLayoutEdge::Right, ownerWidth);
    float paddingBottom = resolvePadding(LFLayoutEdge::Bottom, ownerHeight);

    float contentOwnerWidth = ownerWidth;
    float contentOwnerHeight = ownerHeight;
    if (std::isfinite(contentOwnerWidth)) {
        contentOwnerWidth = std::max(0.0f, contentOwnerWidth - borderLeft - borderRight - paddingLeft - paddingRight);
    }
    if (std::isfinite(contentOwnerHeight)) {
        contentOwnerHeight = std::max(0.0f, contentOwnerHeight - borderTop - borderBottom - paddingTop - paddingBottom);
    }

    float measuredChildrenWidth = 0.0f;
    float measuredChildrenHeight = 0.0f;

    for (const auto& child : getChildren()) {
        if (!child) continue;

        if (child->getLayoutStyle().display == LFDisplay::None) {
            continue;
        }

        ChildLayoutMeta meta = getLayoutMetaForChild(child);
        const LFLayoutStyle& childStyle = child->getLayoutStyle();
        const LFLayoutValue& childWidthStyle = childStyle.width;
        const LFLayoutValue& childHeightStyle = childStyle.height;

        bool hasLeft = isValueDefined(childStyle.position[
            static_cast<size_t>(LFLayoutEdge::Left)]);
        bool hasRight = isValueDefined(childStyle.position[
            static_cast<size_t>(LFLayoutEdge::Right)]);
        bool hasTop = isValueDefined(childStyle.position[
            static_cast<size_t>(LFLayoutEdge::Top)]);
        bool hasBottom = isValueDefined(childStyle.position[
            static_cast<size_t>(LFLayoutEdge::Bottom)]);
        bool widthPercent100 = childWidthStyle.unit == LFLayoutUnit::Percent &&
                               almostEqual(childWidthStyle.value, 100.0f);
        bool heightPercent100 = childHeightStyle.unit == LFLayoutUnit::Percent &&
                                almostEqual(childHeightStyle.value, 100.0f);
        bool matchParentWidth = meta.align == LFBoxAlign::MatchParent || widthPercent100 || (hasLeft && hasRight);
        bool matchParentHeight = meta.align == LFBoxAlign::MatchParent || heightPercent100 || (hasTop && hasBottom);

        float childOwnerWidth = resolveChildMeasureSize(childWidthStyle, contentOwnerWidth);
        float childOwnerHeight = resolveChildMeasureSize(childHeightStyle, contentOwnerHeight);

        // 关键策略：wrap轴上auto子节点应按自然尺寸测量，不应强制吃满父尺寸
        if (!std::isfinite(childOwnerWidth)) {
            childOwnerWidth = (wrapWidth && !matchParentWidth)
                                  ? std::numeric_limits<float>::quiet_NaN()
                                  : contentOwnerWidth;
        }
        if (!std::isfinite(childOwnerHeight)) {
            childOwnerHeight = (wrapHeight && !matchParentHeight)
                                   ? std::numeric_limits<float>::quiet_NaN()
                                   : contentOwnerHeight;
        }

        child->calculateLayout(childOwnerWidth, childOwnerHeight);

        float childWidth = child->getLayoutWidth();
        float childHeight = child->getLayoutHeight();

        float marginStart = meta.marginStart +
                            resolveMargin(child, LFLayoutEdge::Left, contentOwnerWidth);
        float marginTop = meta.marginTop +
                          resolveMargin(child, LFLayoutEdge::Top, contentOwnerHeight);
        float marginEnd = meta.marginEnd +
                          resolveMargin(child, LFLayoutEdge::Right, contentOwnerWidth);
        float marginBottom = meta.marginBottom +
                             resolveMargin(child, LFLayoutEdge::Bottom, contentOwnerHeight);

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
        LFNode::setWidth(measuredWidth);
        m_restoreAutoWidth = true;
    }
    if (wrapHeight) {
        LFNode::setHeight(measuredHeight);
        m_restoreAutoHeight = true;
    }
}

void LFBox::onAfterCalculateLayout() {
    if (m_restoreAutoWidth) {
        LFNode::wrapContentWidth();
        m_restoreAutoWidth = false;
    }
    if (m_restoreAutoHeight) {
        LFNode::wrapContentHeight();
        m_restoreAutoHeight = false;
    }
}

LFBox::Ptr LFBox::create() {
    return std::make_shared<LFBox>();
}
