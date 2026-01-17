//
// Created by Chen Tong on 2026/1/17.
//

#include "LFText.h"

// 初始化静态成员
NVGcontext *LFText::s_measureContext = nullptr;

LFText::LFText() {
    // 将当前实例指针存入 Yoga 节点上下文，供静态 measure 函数找回
    YGNodeSetContext(m_ygNode, this);
    // 开启 Yoga 的测量功能
    YGNodeSetMeasureFunc(m_ygNode, LFText::measure);
}

void LFText::setText(const std::string &text) {
    if (m_text == text) return;
    m_text = text;
    // TODO: 内容改变必须标记 Yoga 节点为脏，否则布局不会重算
    YGNodeMarkDirty(m_ygNode);
    markDirty();
}

void LFText::setFontSize(float size) {
    float safeSize = std::max(1.0f, size);
    if (m_fontSize == safeSize) return;
    m_fontSize = safeSize;
    YGNodeMarkDirty(m_ygNode);
    markDirty();
}

void LFText::setTextColor(uint32_t color) {
    m_textColor = color; // 颜色改变不需要重算布局，只需重绘
}

void LFText::setLineHeight(float lineHeight) {
    if (m_lineHeight == lineHeight) return;
    m_lineHeight = lineHeight;
    YGNodeMarkDirty(m_ygNode);
    markDirty();
}

void LFText::setFontFamily(const std::string &family) {
    if (m_fontFamily == family) return;
    m_fontFamily = family;
    YGNodeMarkDirty(m_ygNode);
    markDirty();
}

/**
 * 当父容器是 Flex 或有固定宽度时，Yoga 会传入 width。
 * 我们需要告诉 Yoga 这段文字在有限宽度下折行后的高度。
 */
YGSize LFText::measure(YGNodeConstRef node, float width, YGMeasureMode widthMode,
                       float height, YGMeasureMode heightMode) {

    LFText *textNode = static_cast<LFText *>(YGNodeGetContext(node));
    NVGcontext *vg = s_measureContext;

    // 如果没有测量上下文或文字为空，返回 0
    if (!vg || textNode->m_text.empty()) return {0, 0};

    nvgSave(vg); // 保护测量上下文状态
    nvgFontSize(vg, textNode->m_fontSize);
    nvgFontFace(vg, textNode->m_fontFamily.c_str());
    nvgTextLineHeight(vg, textNode->m_lineHeight);

    float bounds[4];
    YGSize result;
    if (widthMode == YGMeasureModeUndefined) {
        // 不限制宽度
        nvgTextBounds(vg, 0, 0, textNode->m_text.c_str(), nullptr, bounds);
        result = {(float) ceil(bounds[2] - bounds[0]), (float) ceil(bounds[3] - bounds[1])};
    } else if (widthMode == YGMeasureModeExactly) {
        // 设置固定宽度
        nvgTextBoxBounds(vg, 0, 0, width, textNode->m_text.c_str(), nullptr, bounds);
        result = {width, (float) ceil(bounds[3] - bounds[1])};
    } else {
        // 设置最大宽度
        nvgTextBoxBounds(vg, 0, 0, width, textNode->m_text.c_str(), nullptr, bounds);
        result = {(float) ceil(bounds[2] - bounds[0]), (float) ceil(bounds[3] - bounds[1])};
    }

    nvgRestore(vg);
    return result;
}

void LFText::onDraw(NVGcontext *vg) {
    if (m_text.empty()) return;

    // 应用文本样式
    nvgFontSize(vg, m_fontSize);
    nvgFontFace(vg, m_fontFamily.c_str());
    nvgFillColor(vg, colorToNVG(m_textColor));
    nvgTextLineHeight(vg, m_lineHeight);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    // 获取布局计算出的宽度，用于自动折行绘制
    float layoutW = getLayoutWidth();

    /**
     * 使用 nvgTextBox 而非 nvgText
     * 这样可以确保如果文字超过 getLayoutWidth()，会自动进行排版折行。
     */
    nvgTextBox(vg, 0, 0, layoutW, m_text.c_str(), nullptr);
}
