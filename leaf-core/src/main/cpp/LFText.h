//
// Created by Chen Tong on 2026/1/17.
//

#ifndef LEAF_LFTEXT_H
#define LEAF_LFTEXT_H

#include "LFRenderNode.h"

/**
 * LFText 是 Leaf 引擎的生产级文本组件。
 * 它直接继承自 LFRenderNode 以保证最轻量级的内存足迹。
 */
class LFText : public LFRenderNode {
public:
    LFText();
    virtual ~LFText() = default;

    // --- 生产级属性设置 ---
    void setText(const std::string& text);
    void setFontSize(float size);
    void setTextColor(uint32_t color);
    void setLineHeight(float lineHeight);
    void setFontFamily(const std::string& family);

    /**
     * 设置全局测量上下文
     * 生产环境必备：确保 Yoga 在计算布局时能准确测量文字边界
     */
    static void setMeasureContext(NVGcontext* vg) { s_measureContext = vg; }

protected:
    /**
     * 落地具体的文本绘制
     * 采用 nvgTextBox 以支持自动换行
     */
    void onDraw(NVGcontext* vg) override;

private:
    // Yoga 测量回调函数
    static YGSize measure(YGNodeConstRef node, float width, YGMeasureMode widthMode,
                          float height, YGMeasureMode heightMode);

    // 成员变量
    std::string m_text;
    float m_fontSize = 16.0f;
    uint32_t m_textColor = 0xFF000000;
    float m_lineHeight = 1.2f;
    std::string m_fontFamily = "sans"; // 默认对应加载的字体名

    // 静态测量上下文
    static NVGcontext* s_measureContext;
};

#endif // LEAF_LFTEXT_H
