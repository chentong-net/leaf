#include "view/layout/LFLinear.h"

LFLinear::LFLinear() {
    // 初始化默认状态：垂直、起点对齐、拉伸
    setOrientation(LFOrientation::Vertical);
    updateFlexLayout();
}

void LFLinear::setOrientation(LFOrientation orientation) {
    if (m_orientation == orientation) return;
    m_orientation = orientation;
    updateFlexLayout();
}

void LFLinear::setGravity(LFAlignment main, LFAlignment cross) {
    if (m_mainAlign == main && m_crossAlign == cross) return;
    m_mainAlign = main;
    m_crossAlign = cross;
    updateFlexLayout();
}

void LFLinear::setDistribution(LFDistribution distribution) {
    if (m_distribution == distribution) return;
    m_distribution = distribution;
    updateFlexLayout();
}

void LFLinear::setSpacing(float spacing) {
    setGap(LFGutter::All, spacing);
}

void LFLinear::setRowSpacing(float spacing) {
    setGap(LFGutter::Row, spacing);
}

void LFLinear::setReverse(bool reverse) {
    if (m_isReverse == reverse) return;
    m_isReverse = reverse;
    updateFlexLayout();
}

void LFLinear::updateFlexLayout() {
    // 1. 翻译 FlexDirection (考虑 Orientation 和 Reverse)
    if (m_orientation == LFOrientation::Vertical) {
        setFlexDirection(m_isReverse
                             ? LFFlexDirection::ColumnReverse
                             : LFFlexDirection::Column);
    } else {
        setFlexDirection(m_isReverse
                             ? LFFlexDirection::RowReverse
                             : LFFlexDirection::Row);
    }

    // 2. 翻译 Main Axis Alignment (JustifyContent)
    LFJustify justify = LFJustify::FlexStart;

    // 优先级：Distribution (分布) > Gravity (重力/对齐)
    if (m_distribution != LFDistribution::Pack) {
        switch (m_distribution) {
            case LFDistribution::SpaceBetween: justify = LFJustify::SpaceBetween; break;
            case LFDistribution::SpaceAround:  justify = LFJustify::SpaceAround; break;
            case LFDistribution::SpaceEvenly:  justify = LFJustify::SpaceEvenly; break;
            default: break;
        }
    } else {
        // 紧凑模式下，根据 MainGravity 决定
        // 注意：Reverse 模式下，Start/End 的视觉方向需要反转语义，以符合人类直觉
        switch (m_mainAlign) {
            case LFAlignment::Start:
                justify = m_isReverse ? LFJustify::FlexEnd : LFJustify::FlexStart;
                break;
            case LFAlignment::Center:
                justify = LFJustify::Center;
                break;
            case LFAlignment::End:
                justify = m_isReverse ? LFJustify::FlexStart : LFJustify::FlexEnd;
                break;
            default: justify = LFJustify::FlexStart; break;
        }
    }
    setJustifyContent(justify);

    // 3. 翻译 Cross Axis Alignment (AlignItems)
    LFAlign align = LFAlign::Stretch;
    switch (m_crossAlign) {
        case LFAlignment::Start:    align = LFAlign::FlexStart; break;
        case LFAlignment::Center:   align = LFAlign::Center; break;
        case LFAlignment::End:      align = LFAlign::FlexEnd; break;
        case LFAlignment::Stretch:  align = LFAlign::Stretch; break;
        case LFAlignment::Baseline: align = LFAlign::Baseline; break;
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
