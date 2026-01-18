//
// Created by Chen Tong on 2026/1/17.
//

#include "LFText.h"

// 初始化静态成员
NVGcontext *LFText::s_measureContext = nullptr;

LFText::LFText() {
    // 开启 Yoga 的测量功能
    // 只有叶子节点（如 Text, Image）才需要自定义 MeasureFunc
    YGNodeSetMeasureFunc(getYGNode(), LFText::measure);
}

void LFText::setText(const std::string &text) {
    if (m_text == text) return;
    m_text = text;
    // 内容改变，必须标记 Yoga 节点为脏，触发重新测量 (Measure)
    YGNodeMarkDirty(getYGNode());
    // 同时也标记自身为脏，触发重绘
    markDirty();
}

void LFText::setFontSize(float size) {
    float safeSize = std::max(1.0f, size);
    if (m_fontSize == safeSize) return;
    m_fontSize = safeSize;
    YGNodeMarkDirty(getYGNode());
    markDirty();
}

void LFText::setTextColor(uint32_t color) {
    if (m_textColor == color) return;
    m_textColor = color;
    // 颜色改变不影响布局，只需要重绘，所以不调 YGNodeMarkDirty
    markDirty();
}

void LFText::setLineHeight(float lineHeight) {
    if (m_lineHeight == lineHeight) return;
    m_lineHeight = lineHeight;
    YGNodeMarkDirty(getYGNode());
    markDirty();
}

void LFText::setFontFamily(const std::string &family) {
    if (m_fontFamily == family) return;
    m_fontFamily = family;
    YGNodeMarkDirty(getYGNode());
    markDirty();
}

void LFText::setTextAlign(LFTextAlign align) {
    if (m_textAlign == align) return;
    m_textAlign = align;
    markDirty();
}

/**
 * 测量回调：告诉 Yoga 这段文字到底有多大
 */
YGSize LFText::measure(YGNodeRef node, float width, YGMeasureMode widthMode,
                       float height, YGMeasureMode heightMode) {

    // 1. 获取上下文
    // LFNode 在构造时传入的是 LFNode*，所以先转回 LFNode* 再转 LFText* 安全性最高
    auto *baseNode = static_cast<LFNode *>(YGNodeGetContext(node));
    auto *textNode = static_cast<LFText *>(baseNode);

    NVGcontext *vg = s_measureContext;

    if (!vg || !textNode || textNode->m_text.empty()) {
        return {0, 0};
    }

    // 2. 准备测量环境
    nvgSave(vg);
    nvgFontSize(vg, textNode->m_fontSize);
    nvgFontFace(vg, textNode->m_fontFamily.c_str());
    nvgTextLineHeight(vg, textNode->m_lineHeight);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    float bounds[4];
    YGSize result = {0, 0};

    // 3. 根据 Yoga 的约束模式进行测量
    // nvgTextBoxBounds 用于多行文本测量 (支持自动换行)
    // nvgTextBounds 用于单行文本测量
    if (widthMode == YGMeasureModeExactly) {
        // 宽度固定：计算在该宽度下的高度 (自动换行)
        nvgTextBoxBounds(vg, 0, 0, width, textNode->m_text.c_str(), nullptr, bounds);
        result.width = width;
        result.height = (float) ceil(bounds[3] - bounds[1]);
    } else if (widthMode == YGMeasureModeAtMost) {
        // 宽度受限 (最大不能超过 width)：尝试在该宽度下测量
        nvgTextBoxBounds(vg, 0, 0, width, textNode->m_text.c_str(), nullptr, bounds);
        result.width = (float) ceil(bounds[2] - bounds[0]);
        result.height = (float) ceil(bounds[3] - bounds[1]);
    } else {
        // 宽度无限 (undefined)：通常用单行测量，或者给予一个极大值
        // 这里使用 nvgTextBounds 测量单行自然宽度
        nvgTextBounds(vg, 0, 0, textNode->m_text.c_str(), nullptr, bounds);
        result.width = (float) ceil(bounds[2] - bounds[0]);
        result.height = (float) ceil(bounds[3] - bounds[1]);
    }

    nvgRestore(vg);
    return result;
}

void LFText::onDrawContent(NVGcontext *vg) {
    if (m_text.empty()) return;

    // 1. 设置文本样式
    nvgFontSize(vg, m_fontSize);
    nvgFontFace(vg, m_fontFamily.c_str());
    nvgFillColor(vg, LFNode::colorToNVG(m_textColor)); // 使用 LFNode 的静态工具需要加作用域，或者直接在 LFNode 里暴露 helper
    nvgTextLineHeight(vg, m_lineHeight);

    float paddingL = YGNodeLayoutGetPadding(getYGNode(), YGEdgeLeft);
    float paddingT = YGNodeLayoutGetPadding(getYGNode(), YGEdgeTop);
    float borderL = YGNodeLayoutGetBorder(getYGNode(), YGEdgeLeft);
    float borderT = YGNodeLayoutGetBorder(getYGNode(), YGEdgeTop);

    // 内容区域的左上角
    float contentX = paddingL + borderL;
    float contentY = paddingT + borderT;

    // 内容区域的可用宽度
    float layoutW = getLayoutWidth();
    float contentW = layoutW - contentX - YGNodeLayoutGetPadding(getYGNode(), YGEdgeRight) -
                     YGNodeLayoutGetBorder(getYGNode(), YGEdgeRight);

    if (contentW <= 0) return;

    // 3. 根据对齐方式计算绘制锚点
    int alignFlag = NVG_ALIGN_TOP; // 垂直总是 Top
    float drawX = 0;

    switch (m_textAlign) {
        case LFTextAlign::Left:
            alignFlag |= NVG_ALIGN_LEFT;
            drawX = contentX; // 从左边界开始
            break;

        case LFTextAlign::Center:
            alignFlag |= NVG_ALIGN_CENTER;
            drawX = contentX + (contentW * 0.5f); // 从中心点开始
            break;

        case LFTextAlign::Right:
            alignFlag |= NVG_ALIGN_RIGHT;
            drawX = contentX + contentW; // 从右边界开始
            break;
    }

    // 4. 应用对齐并绘制
    nvgTextAlign(vg, alignFlag);

    // 3. 绘制文本
    // 使用 nvgTextBox 可以保证文字在 layoutW 范围内自动换行
    // 坐标 (0,0) 即可，因为 LFNode::render 已经帮我们 translate 到了正确位置
    nvgTextBox(vg, drawX, contentY, contentW, m_text.c_str(), nullptr);
}
