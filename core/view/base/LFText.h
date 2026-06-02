//
// Created by Chen Tong on 2026/1/17.
//

#ifndef LEAF_LFTEXT_H
#define LEAF_LFTEXT_H

#include "view/base/LFNode.h"

enum class LFTextHAlign { Left, Center, Right };
enum class LFTextVAlign { Top, Center, Bottom };

/**
 * 文本组件
 */
class LFText : public LFNode {
public:
    LFText();
    virtual ~LFText() = default;

    // 文本属性设置
    void setText(const std::string& text);
    void setFontSize(float size);
    void setTextColor(uint32_t color);
    void setLineHeight(float lineHeight);
    void setFontFamily(const std::string& family);
    void setTextHAlign(LFTextHAlign align);
    void setTextVAlign(LFTextVAlign align);
    void setMaxLines(int maxLines);

    /**
     * 设置全局测量上下文
     * 用于在 Yoga 布局阶段（此时可能还没有 render 发生）获取 NanoVG 上下文进行测量
     */
    static void setMeasureContext(NVGcontext* vg) { s_measureContext = vg; }

protected:
    /**
     * 重写内容绘制
     * 这里不需要 nvgTranslate，也不需要画背景，基类都做好了。
     * 我们只需要在 (0,0) 处开始画字即可。
     */
    void onDrawContent(NVGcontext* vg) override;

private:
    // Yoga 测量回调函数
    static YGSize measure(YGNodeRef node, float width, YGMeasureMode widthMode,
                          float height, YGMeasureMode heightMode);
    void invalidateWrapCache();
    const std::string& getWrappedText(NVGcontext* vg, float wrapWidth);

    // 成员变量
    std::string m_text;
    float m_fontSize = 16.0f;
    uint32_t m_textColor = 0xFF000000;
    float m_lineHeight = 1.2f;
    std::string m_fontFamily = "sans";
    LFTextHAlign m_textHAlign = LFTextHAlign::Left;
    LFTextVAlign m_textVAlign = LFTextVAlign::Top;
    int m_maxLines = 0;
    bool m_hasMaxLinesConfigured = false;
    std::string m_wrappedTextCache;
    float m_wrappedWidthCache = -1.0f;
    bool m_wrapCacheValid = false;

    // 静态测量上下文
    static NVGcontext* s_measureContext;
};

#endif // LEAF_LFTEXT_H
