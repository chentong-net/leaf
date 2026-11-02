//
// Created by Chen Tong on 2026/1/17.
//

#ifndef LEAF_LFTEXT_H
#define LEAF_LFTEXT_H

#include "view/base/LFNode.h"
#include "text/LFFont.h"

class LFRenderer;

/**
* Center表示按字形着墨边界进行视觉居中
*/
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
    void setFontWeight(LFFontWeight weight);
    LFFontWeight getFontWeight() const { return m_fontWeight; }
    void setTextHAlign(LFTextHAlign align);
    void setTextVAlign(LFTextVAlign align);
    void setMaxLines(int maxLines);
    void setItalic(bool italic);
    void setUnderline(bool underline);
    void setHoverUnderline(bool enabled);

    /**
     * 设置全局测量上下文
     * 用于在布局测量阶段获取渲染后端
     */
    static void setMeasureContext(LFRenderer* renderer);

protected:
    /**
     * 重写内容绘制
     * 这里不需要处理节点坐标，也不需要画背景，基类都做好了。
     * 我们只需要在 (0,0) 处开始画字即可。
     */
    void onDrawContent(LFPaint& paint) override;

private:
    LFSize measureLayout(const LFMeasureInput& input) const;
    float baselineLayout(const LFBaselineInput& input) const;
    std::string fontFamilyForStyle() const;
    // 成员变量
    std::string m_text;
    float m_fontSize = 16.0f;
    uint32_t m_textColor = 0xFF000000;
    float m_lineHeight = 1.2f;
    std::string m_fontFamily = kLFDefaultFontFamily;
    LFFontWeight m_fontWeight = LFFontWeight::Regular;
    LFTextHAlign m_textHAlign = LFTextHAlign::Left;
    LFTextVAlign m_textVAlign = LFTextVAlign::Top;
    int m_maxLines = 0;
    bool m_hasMaxLinesConfigured = false;
    bool m_italic = false;
    bool m_underline = false;
    bool m_hoverUnderline = false;

    // 静态测量上下文
    static LFRenderer* s_measureContext;
};

#endif // LEAF_LFTEXT_H
