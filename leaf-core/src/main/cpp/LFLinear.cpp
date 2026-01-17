//
// Created by Chen Tong on 2026/1/18.
//

#include "LFLinear.h"

LFLinear::LFLinear() {
    // 默认设置为垂直布局
    YGNodeStyleSetFlexDirection(m_ygNode, YGFlexDirectionColumn);
}

void LFLinear::setDirection(LFLinearDirection dir) {
    m_direction = dir;
    YGNodeStyleSetFlexDirection(m_ygNode,
                                dir == LFLinearDirection::Horizontal ? YGFlexDirectionRow : YGFlexDirectionColumn);
    markDirty();
}

void LFLinear::setJustifyContent(LFJustify justify) {
    switch (justify) {
        case LFJustify::Start:        YGNodeStyleSetJustifyContent(m_ygNode, YGJustifyFlexStart); break;
        case LFJustify::Center:       YGNodeStyleSetJustifyContent(m_ygNode, YGJustifyCenter); break;
        case LFJustify::End:          YGNodeStyleSetJustifyContent(m_ygNode, YGJustifyFlexEnd); break;
        case LFJustify::SpaceBetween: YGNodeStyleSetJustifyContent(m_ygNode, YGJustifySpaceBetween); break;
        case LFJustify::SpaceAround:  YGNodeStyleSetJustifyContent(m_ygNode, YGJustifySpaceAround); break;
    }
    markDirty();
}

void LFLinear::setAlignItems(LFAlign align) {
    switch (align) {
        case LFAlign::Start:   YGNodeStyleSetAlignItems(m_ygNode, YGAlignFlexStart); break;
        case LFAlign::Center:  YGNodeStyleSetAlignItems(m_ygNode, YGAlignCenter); break;
        case LFAlign::End:     YGNodeStyleSetAlignItems(m_ygNode, YGAlignFlexEnd); break;
        case LFAlign::Stretch: YGNodeStyleSetAlignItems(m_ygNode, YGAlignStretch); break;
        default:               YGNodeStyleSetAlignItems(m_ygNode, YGAlignAuto); break;
    }
    markDirty();
}

void LFLinear::setGap(float gap) {
    // 同时设置行间距和列间距，确保在 Row 或 Column 下都生效
    YGNodeStyleSetGap(m_ygNode, YGGutterAll, gap);
    markDirty();
}

void LFLinear::setBackgroundColor(uint32_t color) {
    m_backgroundColor = color;
    markDirty(); // 背景色改变仅需重绘
}

void LFLinear::setBorderRadius(float radius) {
    m_borderRadius = radius;
    markDirty();
}

std::shared_ptr<LFLinear> LFLinear::createHorizontal() {
    auto node = std::make_shared<LFLinear>();
    node->setDirection(LFLinearDirection::Horizontal);
    return node;
}

std::shared_ptr<LFLinear> LFLinear::createVertical() {
    return std::make_shared<LFLinear>();
}

void LFLinear::onDraw(NVGcontext* vg) {
    float w = getLayoutWidth();  // 获取 Yoga 计算出的宽度
    float h = getLayoutHeight(); // 获取 Yoga 计算出的高度

    if (w <= 0 || h <= 0) return;

    // 1. 绘制背景色与圆角
    if ((m_backgroundColor >> 24) & 0xFF) { // 如果 Alpha 不为 0
        nvgBeginPath(vg);
        if (m_borderRadius > 0) {
            nvgRoundedRect(vg, 0, 0, w, h, m_borderRadius);
        } else {
            nvgRect(vg, 0, 0, w, h);
        }
        nvgFillColor(vg, colorToNVG(m_backgroundColor));
        nvgFill(vg);
    }
}
