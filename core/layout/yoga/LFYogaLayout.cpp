#include "layout/yoga/LFYogaLayout.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

YGEdge toYogaEdge(LFLayoutEdge edge) {
    switch (edge) {
        case LFLayoutEdge::Left: return YGEdgeLeft;
        case LFLayoutEdge::Top: return YGEdgeTop;
        case LFLayoutEdge::Right: return YGEdgeRight;
        case LFLayoutEdge::Bottom: return YGEdgeBottom;
        case LFLayoutEdge::Start: return YGEdgeStart;
        case LFLayoutEdge::End: return YGEdgeEnd;
        case LFLayoutEdge::Horizontal: return YGEdgeHorizontal;
        case LFLayoutEdge::Vertical: return YGEdgeVertical;
        case LFLayoutEdge::All: return YGEdgeAll;
    }
    return YGEdgeAll;
}

YGFlexDirection toYogaFlexDirection(LFFlexDirection direction) {
    switch (direction) {
        case LFFlexDirection::Column: return YGFlexDirectionColumn;
        case LFFlexDirection::ColumnReverse: return YGFlexDirectionColumnReverse;
        case LFFlexDirection::Row: return YGFlexDirectionRow;
        case LFFlexDirection::RowReverse: return YGFlexDirectionRowReverse;
    }
    return YGFlexDirectionColumn;
}

YGJustify toYogaJustify(LFJustify justify) {
    switch (justify) {
        case LFJustify::FlexStart: return YGJustifyFlexStart;
        case LFJustify::Center: return YGJustifyCenter;
        case LFJustify::FlexEnd: return YGJustifyFlexEnd;
        case LFJustify::SpaceBetween: return YGJustifySpaceBetween;
        case LFJustify::SpaceAround: return YGJustifySpaceAround;
        case LFJustify::SpaceEvenly: return YGJustifySpaceEvenly;
    }
    return YGJustifyFlexStart;
}

YGAlign toYogaAlign(LFAlign align) {
    switch (align) {
        case LFAlign::Auto: return YGAlignAuto;
        case LFAlign::FlexStart: return YGAlignFlexStart;
        case LFAlign::Center: return YGAlignCenter;
        case LFAlign::FlexEnd: return YGAlignFlexEnd;
        case LFAlign::Stretch: return YGAlignStretch;
        case LFAlign::Baseline: return YGAlignBaseline;
        case LFAlign::SpaceBetween: return YGAlignSpaceBetween;
        case LFAlign::SpaceAround: return YGAlignSpaceAround;
    }
    return YGAlignAuto;
}

YGWrap toYogaWrap(LFWrap wrap) {
    switch (wrap) {
        case LFWrap::NoWrap: return YGWrapNoWrap;
        case LFWrap::Wrap: return YGWrapWrap;
        case LFWrap::WrapReverse: return YGWrapWrapReverse;
    }
    return YGWrapNoWrap;
}

YGPositionType toYogaPositionType(LFPositionType type) {
    switch (type) {
        case LFPositionType::Static: return YGPositionTypeStatic;
        case LFPositionType::Relative: return YGPositionTypeRelative;
        case LFPositionType::Absolute: return YGPositionTypeAbsolute;
    }
    return YGPositionTypeRelative;
}

YGDisplay toYogaDisplay(LFDisplay display) {
    return display == LFDisplay::None ? YGDisplayNone : YGDisplayFlex;
}

YGDirection toYogaDirection(LFDirection direction) {
    switch (direction) {
        case LFDirection::Inherit: return YGDirectionInherit;
        case LFDirection::LTR: return YGDirectionLTR;
        case LFDirection::RTL: return YGDirectionRTL;
    }
    return YGDirectionInherit;
}

YGGutter toYogaGutter(LFGutter gutter) {
    switch (gutter) {
        case LFGutter::Column: return YGGutterColumn;
        case LFGutter::Row: return YGGutterRow;
        case LFGutter::All: return YGGutterAll;
    }
    return YGGutterAll;
}

LFMeasureMode fromYogaMeasureMode(YGMeasureMode mode) {
    switch (mode) {
        case YGMeasureModeExactly: return LFMeasureMode::Exactly;
        case YGMeasureModeAtMost: return LFMeasureMode::AtMost;
        case YGMeasureModeUndefined:
        default: return LFMeasureMode::Undefined;
    }
}

float dimensionValue(const LFLayoutValue& value) {
    return value.unit == LFLayoutUnit::Undefined ? YGUndefined : value.value;
}

void setWidth(YGNodeRef node, const LFLayoutValue& value) {
    switch (value.unit) {
        case LFLayoutUnit::Point:
            YGNodeStyleSetWidth(node, value.value);
            break;
        case LFLayoutUnit::Percent:
            YGNodeStyleSetWidthPercent(node, value.value);
            break;
        case LFLayoutUnit::Auto:
            YGNodeStyleSetWidthAuto(node);
            break;
        case LFLayoutUnit::Undefined:
        default:
            YGNodeStyleSetWidth(node, YGUndefined);
            break;
    }
}

void setHeight(YGNodeRef node, const LFLayoutValue& value) {
    switch (value.unit) {
        case LFLayoutUnit::Point:
            YGNodeStyleSetHeight(node, value.value);
            break;
        case LFLayoutUnit::Percent:
            YGNodeStyleSetHeightPercent(node, value.value);
            break;
        case LFLayoutUnit::Auto:
            YGNodeStyleSetHeightAuto(node);
            break;
        case LFLayoutUnit::Undefined:
        default:
            YGNodeStyleSetHeight(node, YGUndefined);
            break;
    }
}

void setMinWidth(YGNodeRef node, const LFLayoutValue& value) {
    if (value.unit == LFLayoutUnit::Percent) {
        YGNodeStyleSetMinWidthPercent(node, value.value);
    } else {
        YGNodeStyleSetMinWidth(node, dimensionValue(value));
    }
}

void setMinHeight(YGNodeRef node, const LFLayoutValue& value) {
    if (value.unit == LFLayoutUnit::Percent) {
        YGNodeStyleSetMinHeightPercent(node, value.value);
    } else {
        YGNodeStyleSetMinHeight(node, dimensionValue(value));
    }
}

void setMaxWidth(YGNodeRef node, const LFLayoutValue& value) {
    if (value.unit == LFLayoutUnit::Percent) {
        YGNodeStyleSetMaxWidthPercent(node, value.value);
    } else {
        YGNodeStyleSetMaxWidth(node, dimensionValue(value));
    }
}

void setMaxHeight(YGNodeRef node, const LFLayoutValue& value) {
    if (value.unit == LFLayoutUnit::Percent) {
        YGNodeStyleSetMaxHeightPercent(node, value.value);
    } else {
        YGNodeStyleSetMaxHeight(node, dimensionValue(value));
    }
}

void setFlexBasis(YGNodeRef node, const LFLayoutValue& value) {
    switch (value.unit) {
        case LFLayoutUnit::Point:
            YGNodeStyleSetFlexBasis(node, value.value);
            break;
        case LFLayoutUnit::Percent:
            YGNodeStyleSetFlexBasisPercent(node, value.value);
            break;
        case LFLayoutUnit::Auto:
        case LFLayoutUnit::Undefined:
        default:
            YGNodeStyleSetFlexBasisAuto(node);
            break;
    }
}

void setMargin(YGNodeRef node,
               LFLayoutEdge edge,
               const LFLayoutValue& value) {
    const YGEdge yogaEdge = toYogaEdge(edge);
    switch (value.unit) {
        case LFLayoutUnit::Point:
            YGNodeStyleSetMargin(node, yogaEdge, value.value);
            break;
        case LFLayoutUnit::Percent:
            YGNodeStyleSetMarginPercent(node, yogaEdge, value.value);
            break;
        case LFLayoutUnit::Auto:
            YGNodeStyleSetMarginAuto(node, yogaEdge);
            break;
        case LFLayoutUnit::Undefined:
        default:
            YGNodeStyleSetMargin(node, yogaEdge, YGUndefined);
            break;
    }
}

void setPadding(YGNodeRef node,
                LFLayoutEdge edge,
                const LFLayoutValue& value) {
    const YGEdge yogaEdge = toYogaEdge(edge);
    switch (value.unit) {
        case LFLayoutUnit::Point:
            YGNodeStyleSetPadding(node, yogaEdge, value.value);
            break;
        case LFLayoutUnit::Percent:
            YGNodeStyleSetPaddingPercent(node, yogaEdge, value.value);
            break;
        case LFLayoutUnit::Auto:
        case LFLayoutUnit::Undefined:
        default:
            YGNodeStyleSetPadding(node, yogaEdge, YGUndefined);
            break;
    }
}

void setPosition(YGNodeRef node,
                 LFLayoutEdge edge,
                 const LFLayoutValue& value) {
    const YGEdge yogaEdge = toYogaEdge(edge);
    switch (value.unit) {
        case LFLayoutUnit::Point:
            YGNodeStyleSetPosition(node, yogaEdge, value.value);
            break;
        case LFLayoutUnit::Percent:
            YGNodeStyleSetPositionPercent(node, yogaEdge, value.value);
            break;
        case LFLayoutUnit::Auto:
        case LFLayoutUnit::Undefined:
        default:
            YGNodeStyleSetPosition(node, yogaEdge, YGUndefined);
            break;
    }
}

void setBorder(YGNodeRef node,
               LFLayoutEdge edge,
               const LFLayoutValue& value) {
    YGNodeStyleSetBorder(node, toYogaEdge(edge), dimensionValue(value));
}

LFEdgeInsets readInsets(YGNodeRef node, bool border) {
    LFEdgeInsets insets;
    const auto get = [node, border](YGEdge edge) {
        return border
                   ? YGNodeLayoutGetBorder(node, edge)
                   : YGNodeLayoutGetPadding(node, edge);
    };
    insets.left = get(YGEdgeLeft);
    insets.top = get(YGEdgeTop);
    insets.right = get(YGEdgeRight);
    insets.bottom = get(YGEdgeBottom);
    return insets;
}

} // namespace

LFYogaLayout::LFYogaLayout() {
    m_node = YGNodeNew();
    YGNodeSetContext(m_node, this);
}

LFYogaLayout::~LFYogaLayout() {
    if (m_node) {
        YGNodeFree(m_node);
    }
}

void LFYogaLayout::setStyle(const LFLayoutStyle& style) {
    YGNodeStyleSetDirection(m_node, toYogaDirection(style.direction));
    YGNodeStyleSetFlexDirection(m_node, toYogaFlexDirection(style.flexDirection));
    YGNodeStyleSetJustifyContent(m_node, toYogaJustify(style.justifyContent));
    YGNodeStyleSetAlignContent(m_node, toYogaAlign(style.alignContent));
    YGNodeStyleSetAlignItems(m_node, toYogaAlign(style.alignItems));
    YGNodeStyleSetAlignSelf(m_node, toYogaAlign(style.alignSelf));
    YGNodeStyleSetPositionType(m_node, toYogaPositionType(style.positionType));
    YGNodeStyleSetFlexWrap(m_node, toYogaWrap(style.flexWrap));
    YGNodeStyleSetDisplay(m_node, toYogaDisplay(style.display));

    YGNodeStyleSetFlexGrow(m_node, style.flexGrow);
    YGNodeStyleSetFlexShrink(m_node, style.flexShrink);
    setFlexBasis(m_node, style.flexBasis);
    YGNodeStyleSetAspectRatio(m_node, style.aspectRatio);

    setWidth(m_node, style.width);
    setHeight(m_node, style.height);
    setMinWidth(m_node, style.minWidth);
    setMinHeight(m_node, style.minHeight);
    setMaxWidth(m_node, style.maxWidth);
    setMaxHeight(m_node, style.maxHeight);

    for (size_t index = 0; index < kLFLayoutEdgeCount; ++index) {
        const auto edge = static_cast<LFLayoutEdge>(index);
        setMargin(m_node, edge, style.margin[index]);
        setPadding(m_node, edge, style.padding[index]);
        setBorder(m_node, edge, style.border[index]);
        setPosition(m_node, edge, style.position[index]);
    }

    for (size_t index = 0; index < style.gap.size(); ++index) {
        YGNodeStyleSetGap(
            m_node,
            toYogaGutter(static_cast<LFGutter>(index)),
            style.gap[index]
        );
    }
}

void LFYogaLayout::insertChild(LFLayout& child, size_t index) {
    auto* yogaChild = dynamic_cast<LFYogaLayout*>(&child);
    if (!yogaChild) {
        return;
    }
    YGNodeInsertChild(m_node, yogaChild->m_node, static_cast<uint32_t>(index));
}

void LFYogaLayout::removeChild(LFLayout& child) {
    auto* yogaChild = dynamic_cast<LFYogaLayout*>(&child);
    if (!yogaChild) {
        return;
    }
    YGNodeRemoveChild(m_node, yogaChild->m_node);
}

void LFYogaLayout::setMeasureFunction(LFMeasureFunction function) {
    m_measureFunction = std::move(function);
    YGNodeSetMeasureFunc(m_node, m_measureFunction ? measureCallback : nullptr);
}

void LFYogaLayout::setBaselineFunction(LFBaselineFunction function) {
    m_baselineFunction = std::move(function);
    YGNodeSetBaselineFunc(m_node, m_baselineFunction ? baselineCallback : nullptr);
}

void LFYogaLayout::markDirty() {
    if (YGNodeHasMeasureFunc(m_node)) {
        YGNodeMarkDirty(m_node);
    }
}

void LFYogaLayout::calculate(const LFLayoutConstraints& constraints) {
    YGNodeCalculateLayout(
        m_node,
        constraints.width,
        constraints.height,
        YGDirectionLTR
    );
}

void LFYogaLayout::updateFrame() {
    m_frame.x = YGNodeLayoutGetLeft(m_node);
    m_frame.y = YGNodeLayoutGetTop(m_node);
    m_frame.width = YGNodeLayoutGetWidth(m_node);
    m_frame.height = YGNodeLayoutGetHeight(m_node);
    m_frame.padding = readInsets(m_node, false);
    m_frame.border = readInsets(m_node, true);
}

const LFLayoutFrame& LFYogaLayout::getFrame() const {
    return m_frame;
}

YGSize LFYogaLayout::measureCallback(YGNodeRef node,
                                     float width,
                                     YGMeasureMode widthMode,
                                     float height,
                                     YGMeasureMode heightMode) {
    auto* layout = static_cast<LFYogaLayout*>(YGNodeGetContext(node));
    if (!layout || !layout->m_measureFunction) {
        return {0.0f, 0.0f};
    }

    LFMeasureInput input;
    input.width = {width, fromYogaMeasureMode(widthMode)};
    input.height = {height, fromYogaMeasureMode(heightMode)};
    const LFSize size = layout->m_measureFunction(input);
    return {size.width, size.height};
}

float LFYogaLayout::baselineCallback(YGNodeRef node, float width, float height) {
    auto* layout = static_cast<LFYogaLayout*>(YGNodeGetContext(node));
    if (!layout || !layout->m_baselineFunction) {
        return height;
    }

    LFBaselineInput input;
    input.width = width;
    input.height = height;
    input.padding = readInsets(node, false);
    input.border = readInsets(node, true);
    return layout->m_baselineFunction(input);
}
