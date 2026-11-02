#ifndef LEAF_LFLAYOUTSTYLE_H
#define LEAF_LFLAYOUTSTYLE_H

#include <array>
#include <cstddef>
#include <cmath>
#include <limits>

enum class LFLayoutUnit {
    Undefined,
    Point,
    Percent,
    Auto
};

enum class LFLayoutEdge {
    Left,
    Top,
    Right,
    Bottom,
    Start,
    End,
    Horizontal,
    Vertical,
    All
};

constexpr size_t kLFLayoutEdgeCount = 9;

enum class LFFlexDirection {
    Column,
    ColumnReverse,
    Row,
    RowReverse
};

enum class LFJustify {
    FlexStart,
    Center,
    FlexEnd,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

enum class LFAlign {
    Auto,
    FlexStart,
    Center,
    FlexEnd,
    Stretch,
    Baseline,
    SpaceBetween,
    SpaceAround
};

enum class LFWrap {
    NoWrap,
    Wrap,
    WrapReverse
};

enum class LFPositionType {
    Static,
    Relative,
    Absolute
};

enum class LFDisplay {
    Flex,
    None
};

enum class LFDirection {
    Inherit,
    LTR,
    RTL
};

enum class LFGutter {
    Column,
    Row,
    All
};

struct LFLayoutValue {
    float value = 0.0f;
    LFLayoutUnit unit = LFLayoutUnit::Undefined;

    constexpr LFLayoutValue() = default;
    constexpr LFLayoutValue(float value, LFLayoutUnit unit)
        : value(value), unit(unit) {}
};

inline bool operator==(const LFLayoutValue& left, const LFLayoutValue& right) {
    return left.unit == right.unit &&
           (left.unit == LFLayoutUnit::Undefined || left.value == right.value);
}

inline bool operator!=(const LFLayoutValue& left, const LFLayoutValue& right) {
    return !(left == right);
}

inline bool LFLayoutValueIsDefined(const LFLayoutValue& value) {
    return value.unit != LFLayoutUnit::Undefined;
}

inline float LFLayoutResolveValue(const LFLayoutValue& value,
                                  float ownerSize,
                                  float fallback = 0.0f) {
    switch (value.unit) {
        case LFLayoutUnit::Point:
            return value.value;
        case LFLayoutUnit::Percent:
            return std::isfinite(ownerSize)
                       ? ownerSize * value.value / 100.0f
                       : fallback;
        case LFLayoutUnit::Auto:
        case LFLayoutUnit::Undefined:
        default:
            return fallback;
    }
}

inline LFLayoutValue LFLayoutResolveEdgeValue(
        const std::array<LFLayoutValue, kLFLayoutEdgeCount>& values,
        LFLayoutEdge edge) {
    const auto at = [&values](LFLayoutEdge candidate) -> const LFLayoutValue& {
        return values[static_cast<size_t>(candidate)];
    };

    const LFLayoutValue& direct = at(edge);
    if (LFLayoutValueIsDefined(direct)) {
        return direct;
    }

    if (edge == LFLayoutEdge::Left || edge == LFLayoutEdge::Right) {
        const LFLayoutValue& horizontal = at(LFLayoutEdge::Horizontal);
        if (LFLayoutValueIsDefined(horizontal)) {
            return horizontal;
        }
    } else if (edge == LFLayoutEdge::Top || edge == LFLayoutEdge::Bottom) {
        const LFLayoutValue& vertical = at(LFLayoutEdge::Vertical);
        if (LFLayoutValueIsDefined(vertical)) {
            return vertical;
        }
    }

    return at(LFLayoutEdge::All);
}

struct LFLayoutStyle {
    LFLayoutValue width{0.0f, LFLayoutUnit::Auto};
    LFLayoutValue height{0.0f, LFLayoutUnit::Auto};
    LFLayoutValue minWidth;
    LFLayoutValue minHeight;
    LFLayoutValue maxWidth;
    LFLayoutValue maxHeight;
    LFLayoutValue flexBasis{0.0f, LFLayoutUnit::Auto};

    LFFlexDirection flexDirection = LFFlexDirection::Column;
    LFJustify justifyContent = LFJustify::FlexStart;
    LFAlign alignContent = LFAlign::FlexStart;
    LFAlign alignItems = LFAlign::Stretch;
    LFAlign alignSelf = LFAlign::Auto;
    LFPositionType positionType = LFPositionType::Relative;
    LFWrap flexWrap = LFWrap::NoWrap;
    LFDisplay display = LFDisplay::Flex;
    LFDirection direction = LFDirection::Inherit;

    float flexGrow = 0.0f;
    float flexShrink = 0.0f;
    float aspectRatio = std::numeric_limits<float>::quiet_NaN();

    std::array<LFLayoutValue, kLFLayoutEdgeCount> margin{};
    std::array<LFLayoutValue, kLFLayoutEdgeCount> padding{};
    std::array<LFLayoutValue, kLFLayoutEdgeCount> border{};
    std::array<LFLayoutValue, kLFLayoutEdgeCount> position{};
    std::array<float, 3> gap{
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN()
    };
};

#endif // LEAF_LFLAYOUTSTYLE_H
