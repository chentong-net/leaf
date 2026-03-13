//
// Created by Chen Tong on 2026/1/17.
//

#include "view/base/LFText.h"
#include <cmath>

// 初始化静态成员
NVGcontext *LFText::s_measureContext = nullptr;

LFText::LFText() {
    // 开启 Yoga 的测量功能
    // 只有叶子节点（如 Text, Image）才需要自定义 MeasureFunc
    YGNodeSetMeasureFunc(getYGNode(), LFText::measure);
}

void LFText::invalidateWrapCache() {
    m_wrapCacheValid = false;
    m_wrappedWidthCache = -1.0f;
    m_wrappedTextCache.clear();
}

const std::string& LFText::getWrappedText(NVGcontext* vg, float wrapWidth) {
    if (m_text.empty() || !vg || wrapWidth <= 0.0f) {
        return m_text;
    }

    if (m_wrapCacheValid && std::fabs(m_wrappedWidthCache - wrapWidth) < 0.5f) {
        return m_wrappedTextCache;
    }

    m_wrappedTextCache.clear();
    m_wrappedTextCache.reserve(m_text.size() + 16);

    constexpr int kBatchRows = 16;
    NVGtextRow rows[kBatchRows];
    const char* cursor = m_text.c_str();

    while (cursor && *cursor != '\0') {
        int nrows = nvgTextBreakLines(vg, cursor, nullptr, wrapWidth, rows, kBatchRows);
        if (nrows <= 0) break;

        for (int i = 0; i < nrows; ++i) {
            const NVGtextRow& row = rows[i];
            if (row.end > row.start) {
                m_wrappedTextCache.append(row.start, static_cast<size_t>(row.end - row.start));
            }
            if (row.next && *row.next != '\0') {
                m_wrappedTextCache.push_back('\n');
            }
        }

        const char* next = rows[nrows - 1].next;
        if (!next || next == cursor) break;
        cursor = next;
    }

    if (m_wrappedTextCache.empty()) {
        m_wrappedTextCache = m_text;
    }

    m_wrapCacheValid = true;
    m_wrappedWidthCache = wrapWidth;
    return m_wrappedTextCache;
}

void LFText::setText(const std::string &text) {
    if (m_text == text) return;
    m_text = text;
    invalidateWrapCache();
    // 内容改变，必须标记 Yoga 节点为脏，触发重新测量 (Measure)
    YGNodeMarkDirty(getYGNode());
    // 同时也标记自身为脏，触发重绘
    markDirty();
}

void LFText::setFontSize(float size) {
    float safeSize = std::max(1.0f, size);
    if (m_fontSize == safeSize) return;
    m_fontSize = safeSize;
    invalidateWrapCache();
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
    invalidateWrapCache();
    YGNodeMarkDirty(getYGNode());
    markDirty();
}

void LFText::setFontFamily(const std::string &family) {
    if (m_fontFamily == family) return;
    m_fontFamily = family;
    invalidateWrapCache();
    YGNodeMarkDirty(getYGNode());
    markDirty();
}

void LFText::setTextHAlign(LFTextHAlign align) {
    if (m_textHAlign == align) return;
    m_textHAlign = align;
    markDirty();
}

void LFText::setTextVAlign(LFTextVAlign align) {
    if (m_textVAlign == align) return;
    m_textVAlign = align;
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

    // 计算 Padding 和 Border
    float pl = YGNodeLayoutGetPadding(node, YGEdgeLeft);
    float pr = YGNodeLayoutGetPadding(node, YGEdgeRight);
    float pt = YGNodeLayoutGetPadding(node, YGEdgeTop);
    float pb = YGNodeLayoutGetPadding(node, YGEdgeBottom);

    float bl = YGNodeLayoutGetBorder(node, YGEdgeLeft);
    float br = YGNodeLayoutGetBorder(node, YGEdgeRight);
    float bt = YGNodeLayoutGetBorder(node, YGEdgeTop);
    float bb = YGNodeLayoutGetBorder(node, YGEdgeBottom);

    float extraW = pl + pr + bl + br;
    float extraH = pt + pb + bt + bb;

    // 2. 准备测量环境
    nvgSave(vg);
    nvgFontSize(vg, textNode->m_fontSize);
    nvgFontFace(vg, textNode->m_fontFamily.c_str());
    nvgTextLineHeight(vg, textNode->m_lineHeight);

    float bounds[4];
    YGSize result = {0, 0};
    const float WRAP_BUFFER = std::max(8.0f, textNode->m_fontSize * 0.1f); // 安全像素，避免浮点误差导致边界误判

    // 统一以 LEFT|TOP 测量，再由外部逻辑处理对齐。
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    if (widthMode == YGMeasureModeExactly) {
        // 宽度固定：计算在该宽度下的高度 (自动换行)
        nvgTextBoxBounds(vg, 0, 0, width, textNode->m_text.c_str(), nullptr, bounds);
        result.width = width;
        result.height = (float) ceil(bounds[3] - bounds[1]);
    } else if (widthMode == YGMeasureModeAtMost) {
        // 宽度受限 (最大不能超过 width)：尝试在该宽度下测量
        nvgTextBoxBounds(vg, 0, 0, width, textNode->m_text.c_str(), nullptr, bounds);
        result.width = (float) ceil(bounds[2] - bounds[0]) + WRAP_BUFFER;
        result.height = (float) ceil(bounds[3] - bounds[1]);
    } else {
        // 宽度无限：保留原始文本自然宽度测量
        nvgTextBounds(vg, 0, 0, textNode->m_text.c_str(), nullptr, bounds);
        result.width = (float) ceil(bounds[2] - bounds[0]) + WRAP_BUFFER;
        result.height = (float) ceil(bounds[3] - bounds[1]);
    }

    nvgRestore(vg);

    result.width += extraW;
    result.height += extraH;
    return result;
}

void LFText::onDrawContent(NVGcontext *vg) {
    if (m_text.empty()) return;

    // 设置文本样式
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
    float layoutH = getLayoutHeight();
    float contentW = layoutW - contentX - YGNodeLayoutGetPadding(getYGNode(), YGEdgeRight) -
                     YGNodeLayoutGetBorder(getYGNode(), YGEdgeRight);
    float contentH = layoutH - contentY - YGNodeLayoutGetPadding(getYGNode(), YGEdgeBottom) - YGNodeLayoutGetBorder(getYGNode(), YGEdgeBottom);

    if (contentW <= 0) return;

    // 分行前固定使用 LEFT|TOP，避免受外部对齐状态影响。
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    // 生成内部换行缓存文本（不修改 m_text 原始内容）
    const std::string& wrappedText = getWrappedText(vg, contentW);

    // 根据对齐方式计算绘制锚点
    int alignFlag;
    switch (m_textHAlign) {
        case LFTextHAlign::Left:
            alignFlag = NVG_ALIGN_LEFT;
            break;
        case LFTextHAlign::Center:
            alignFlag = NVG_ALIGN_CENTER;
            break;
        case LFTextHAlign::Right:
            alignFlag = NVG_ALIGN_RIGHT;
            break;
    }

    // 先测量文本总高度，再计算 Y 偏移
    float bounds[4];
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP); // 测量时总是用 Top
    nvgTextBoxBounds(vg, 0, 0, contentW, wrappedText.c_str(), nullptr, bounds);
    float textHeight = bounds[3] - bounds[1];
    // 根据对齐方式计算 Y 轴偏移量
    float drawY = contentY;
    switch (m_textVAlign) {
        case LFTextVAlign::Top:
            drawY = contentY;
            break;
        case LFTextVAlign::Center:
            // 居中公式：起始点 + (容器高 - 内容高)/2
            drawY = contentY + (contentH - textHeight) * 0.5f;
            break;
        case LFTextVAlign::Bottom:
            // 底部公式：起始点 + (容器高 - 内容高)
            drawY = contentY + (contentH - textHeight);
            break;
    }

    // 应用对齐并绘制
    // 均以 Top 为基准
    nvgTextAlign(vg, alignFlag | NVG_ALIGN_TOP);

    // 使用缓存文本绘制，换行点已提前固定为 '\n'，规避自动换行偏差。
    nvgTextBox(vg, contentX, drawY, contentW, wrappedText.c_str(), nullptr);
}
