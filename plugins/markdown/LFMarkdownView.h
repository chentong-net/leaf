//
// Cross-platform Markdown view composed from existing Leaf components.
//

#ifndef LEAF_LFMARKDOWNVIEW_H
#define LEAF_LFMARKDOWNVIEW_H

#include "text/LFFont.h"
#include "view/layout/LFBox.h"

#include <array>
#include <cstdint>
#include <functional>
#include <string>

class LFLinear;
class LFScrollView;

struct LFMarkdownTheme {
    uint32_t backgroundColor = 0xFFFFFFFF;
    uint32_t textColor = 0xFF1F2937;
    uint32_t boldTextColor = 0xFF0F172A;
    uint32_t italicTextColor = 0xFF374151;
    uint32_t headingColor = 0xFF111827;
    uint32_t linkColor = 0xFF2563EB;
    uint32_t codeTextColor = 0xFFE5E7EB;
    uint32_t inlineCodeTextColor = 0xFFBE123C;
    uint32_t inlineCodeBackgroundColor = 0xFFF3F4F6;
    uint32_t codeBlockBackgroundColor = 0xFF111827;
    uint32_t quoteBackgroundColor = 0xFFF8FAFC;
    uint32_t quoteBarColor = 0xFF60A5FA;
    uint32_t tableBorderColor = 0xFFE5E7EB;
    uint32_t tableHeaderBackgroundColor = 0xFFF3F4F6;
    uint32_t tableBodyBackgroundColor = 0xFFFFFFFF;
    uint32_t dividerColor = 0xFFD1D5DB;

    std::string fontFamily = kLFDefaultFontFamily;
    std::string codeFontFamily = kLFDefaultFontFamily;
    LFFontWeight boldWeight = LFFontWeight::Bold;

    std::array<float, 6> headingFontSizes = {30.0f, 24.0f, 20.0f, 18.0f, 16.0f, 15.0f};
    float bodyFontSize = 15.0f;
    float codeFontSize = 13.0f;
    float lineHeight = 1.55f;
    float contentPadding = 20.0f;
    float blockSpacing = 14.0f;
    float headingSpacing = 12.0f;
    float listMarkerWidth = 18.0f;
    float listMarkerGap = 6.0f;
    float listItemSpacing = 10.0f;
    float listBlockSpacing = 18.0f;
    float quoteBarWidth = 4.0f;
    float quotePadding = 12.0f;
    float imageMaxWidth = 320.0f;
    float imageMaxHeight = 240.0f;
    float inlineImageWidth = 96.0f;
    float inlineImageHeight = 64.0f;
    float tableCellPadding = 9.0f;
    float tableCellMinHeight = 36.0f;
};

class LFMarkdownView : public LFBox {
public:
    using Ptr = std::shared_ptr<LFMarkdownView>;
    using LinkCallback = std::function<void(const std::string&)>;

    static Ptr create();

    LFMarkdownView();
    ~LFMarkdownView() override = default;

    void setMarkdown(const std::string& markdown);
    const std::string& getMarkdown() const { return m_markdown; }

    void setTheme(const LFMarkdownTheme& theme);
    const LFMarkdownTheme& getTheme() const { return m_theme; }

    void setOnLinkTap(LinkCallback callback);

private:
    void rebuild();

    std::string m_markdown;
    LFMarkdownTheme m_theme;
    LinkCallback m_onLinkTap;
    std::shared_ptr<LFScrollView> m_scrollView;
    std::shared_ptr<LFLinear> m_content;
};

#endif // LEAF_LFMARKDOWNVIEW_H
