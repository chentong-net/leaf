#include "LFLinear.h"

LFLinear::LFLinear() {
    // 初始化默认状态：垂直、起点对齐、拉伸
    setOrientation(LFOrientation::Vertical);
    updateYogaLayout();
}

void LFLinear::setOrientation(LFOrientation orientation) {
    if (m_orientation == orientation) return;
    m_orientation = orientation;
    updateYogaLayout();
}

void LFLinear::setGravity(LFAlignment main, LFAlignment cross) {
    if (m_mainAlign == main && m_crossAlign == cross) return;
    m_mainAlign = main;
    m_crossAlign = cross;
    updateYogaLayout();
}

void LFLinear::setDistribution(LFDistribution distribution) {
    if (m_distribution == distribution) return;
    m_distribution = distribution;
    updateYogaLayout();
}

void LFLinear::setSpacing(float spacing) {
    // 语义化的间距设置，直接映射到 Yoga Gap
    YGNodeStyleSetGap(getYGNode(), YGGutterAll, spacing);
    markDirty();
}

void LFLinear::setReverse(bool reverse) {
    if (m_isReverse == reverse) return;
    m_isReverse = reverse;
    updateYogaLayout();
}

void LFLinear::updateYogaLayout() {
    // 1. 翻译 FlexDirection (考虑 Orientation 和 Reverse)
    if (m_orientation == LFOrientation::Vertical) {
        setFlexDirection(m_isReverse ? YGFlexDirectionColumnReverse : YGFlexDirectionColumn);
    } else {
        setFlexDirection(m_isReverse ? YGFlexDirectionRowReverse : YGFlexDirectionRow);
    }

    // 2. 翻译 Main Axis Alignment (JustifyContent)
    YGJustify justify = YGJustifyFlexStart;

    // 优先级：Distribution (分布) > Gravity (重力/对齐)
    if (m_distribution != LFDistribution::Pack) {
        switch (m_distribution) {
            case LFDistribution::SpaceBetween: justify = YGJustifySpaceBetween; break;
            case LFDistribution::SpaceAround:  justify = YGJustifySpaceAround; break;
            case LFDistribution::SpaceEvenly:  justify = YGJustifySpaceEvenly; break;
            default: break;
        }
    } else {
        // 紧凑模式下，根据 MainGravity 决定
        // 注意：Reverse 模式下，Start/End 的视觉方向需要反转语义，以符合人类直觉
        switch (m_mainAlign) {
            case LFAlignment::Start:
                justify = m_isReverse ? YGJustifyFlexEnd : YGJustifyFlexStart;
                break;
            case LFAlignment::Center:
                justify = YGJustifyCenter;
                break;
            case LFAlignment::End:
                justify = m_isReverse ? YGJustifyFlexStart : YGJustifyFlexEnd;
                break;
            default: justify = YGJustifyFlexStart; break;
        }
    }
    setJustifyContent(justify);

    // 3. 翻译 Cross Axis Alignment (AlignItems)
    YGAlign align = YGAlignStretch;
    switch (m_crossAlign) {
        case LFAlignment::Start:    align = YGAlignFlexStart; break;
        case LFAlignment::Center:   align = YGAlignCenter; break;
        case LFAlignment::End:      align = YGAlignFlexEnd; break;
        case LFAlignment::Stretch:  align = YGAlignStretch; break;
        case LFAlignment::Baseline: align = YGAlignBaseline; break;
    }
    setAlignItems(align);
}

// 工厂方法
std::shared_ptr<LFLinear> LFLinear::createVertical() {
    return std::make_shared<LFLinear>();
}

std::shared_ptr<LFLinear> LFLinear::createHorizontal() {
    auto node = std::make_shared<LFLinear>();
    node->setOrientation(LFOrientation::Horizontal);
    return node;
}