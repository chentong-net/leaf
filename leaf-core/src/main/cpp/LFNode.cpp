//
// Created by Chen Tong on 2026/1/17.
//

#include "LFNode.h"

void LFNode::setBackgroundColor(uint32_t color) {
    if (m_backgroundColor == color) return;
    m_backgroundColor = color;
    // 背景色改变通常不需要重新布局，但需要重新绘制
    // 如果引擎有单独的 markNeedsPaint，这里可以调用
}

void LFNode::setCornerRadius(float radius) {
    // 半径不能为负
    float safeRadius = std::max(0.0f, radius);
    if (m_cornerRadius == safeRadius) return;
    m_cornerRadius = safeRadius;
}

void LFNode::setBorder(float width, uint32_t color) {
    float adventureWidth = std::max(0.0f, width);
    if (m_borderWidth == adventureWidth && m_borderColor == color) return;
    m_borderWidth = adventureWidth;
    m_borderColor = color;
}

void LFNode::onDraw(NVGcontext* vg) {
    // 从 Yoga 获取计算好的尺寸
    float w = getLayoutWidth();
    float h = getLayoutHeight();

    // 健壮性检查：如果尺寸太小，直接跳过绘制
    if (w < 0.1f || h < 0.1f) return;

    // 1. 绘制背景
    // 性能优化：只有 Alpha 不为 0 时才调用绘制指令
    if ((m_backgroundColor >> 24) & 0xFF) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 0, 0, w, h, m_cornerRadius);
        nvgFillColor(vg, colorToNVG(m_backgroundColor));
        nvgFill(vg);
    }

    // 2. 绘制边框
    if (m_borderWidth > 0.0f && ((m_borderColor >> 24) & 0xFF)) {
        nvgBeginPath(vg);

        /**
         * NanoVG 的 Stroke 是居中绘制的。
         * 为了让边框看起来在盒子内部（类似 CSS 的 border-box），
         * 我们需要根据边框宽度做 0.5 * width 的缩进。
         */
        float halfStroke = m_borderWidth * 0.5f;
        nvgRoundedRect(vg,
                       halfStroke,
                       halfStroke,
                       w - m_borderWidth,
                       h - m_borderWidth,
                       std::max(0.0f, m_cornerRadius - halfStroke));

        nvgStrokeWidth(vg, m_borderWidth);
        nvgStrokeColor(vg, colorToNVG(m_borderColor));
        nvgStroke(vg);
    }
}
