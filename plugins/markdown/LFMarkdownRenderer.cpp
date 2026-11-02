#include "LFMarkdownRenderer.h"

#include "event/LFEvent.h"
#include "view/base/LFImage.h"
#include "view/base/LFText.h"
#include "view/layout/LFBox.h"

#include <algorithm>
#include <functional>
#include <string>

namespace {

void appendBlock(const std::shared_ptr<LFLinear>& parent,
                 const LFNode::Ptr& child,
                 float spacing) {
    if (!parent || !child) return;
    child->setMargin(YGEdgeBottom, spacing);
    parent->addChild(child);
}

} // namespace

LFMarkdownRenderer::LFMarkdownRenderer(
        const LFMarkdownTheme& theme,
        const LFMarkdownView::LinkCallback& onLinkTap)
    : m_theme(theme)
    , m_onLinkTap(onLinkTap) {
}

std::shared_ptr<LFLinear> LFMarkdownRenderer::render(
        const LFMarkdownDocument& document) const {
    auto root = LFLinear::createVertical();
    root->matchParentWidth();
    root->wrapContentHeight();
    root->setPadding(YGEdgeAll, m_theme.contentPadding);

    for (const LFMarkdownBlock& block : document.blocks) {
        renderBlock(block, root);
    }
    return root;
}

void LFMarkdownRenderer::renderBlock(
        const LFMarkdownBlock& block,
        const std::shared_ptr<LFLinear>& parent) const {
    switch (block.type) {
        case LFMarkdownBlockType::Paragraph:
            renderParagraph(block, parent, m_theme.blockSpacing);
            break;
        case LFMarkdownBlockType::Heading:
            renderHeading(block, parent);
            break;
        case LFMarkdownBlockType::Quote:
            renderQuote(block, parent);
            break;
        case LFMarkdownBlockType::UnorderedList:
        case LFMarkdownBlockType::OrderedList:
            renderList(block, parent, m_theme.listBlockSpacing, false);
            break;
        case LFMarkdownBlockType::CodeBlock:
            renderCodeBlock(block, parent);
            break;
        case LFMarkdownBlockType::ThematicBreak:
            renderThematicBreak(parent);
            break;
        case LFMarkdownBlockType::ListItem:
        case LFMarkdownBlockType::Table:
            break;
    }
}

void LFMarkdownRenderer::renderParagraph(
        const LFMarkdownBlock& block,
        const std::shared_ptr<LFLinear>& parent,
        float bottomSpacing) const {
    const LFMarkdownInline* standaloneImage = nullptr;
    bool hasOtherContent = false;
    for (const LFMarkdownInline& value : block.inlines) {
        if (value.type == LFMarkdownInlineType::Image && !standaloneImage) {
            standaloneImage = &value;
            continue;
        }
        if (value.type == LFMarkdownInlineType::Text &&
            value.text.find_first_not_of(" \t\n") == std::string::npos) {
            continue;
        }
        hasOtherContent = true;
        break;
    }
    if (standaloneImage && !hasOtherContent) {
        renderImageBlock(*standaloneImage, parent);
        return;
    }

    appendBlock(
        parent,
        renderInlineFlow(block.inlines, m_theme.bodyFontSize, m_theme.textColor),
        bottomSpacing
    );
}

void LFMarkdownRenderer::renderHeading(
        const LFMarkdownBlock& block,
        const std::shared_ptr<LFLinear>& parent) const {
    const int index = std::clamp(block.level - 1, 0, 5);
    auto text = std::make_shared<LFText>();
    text->matchParentWidth();
    text->wrapContentHeight();
    text->setFontFamily(m_theme.fontFamily);
    text->setFontSize(m_theme.headingFontSizes[static_cast<size_t>(index)]);
    text->setFontWeight(LFFontWeight::SemiBold);
    text->setLineHeight(1.25f);
    text->setTextColor(m_theme.headingColor);
    text->setText(flattenInline(block.inlines));
    appendBlock(parent, text, m_theme.headingSpacing);
}

void LFMarkdownRenderer::renderQuote(
        const LFMarkdownBlock& block,
        const std::shared_ptr<LFLinear>& parent) const {
    auto row = LFLinear::createHorizontal();
    row->matchParentWidth();
    row->wrapContentHeight();
    row->setGravity(LFAlignment::Start, LFAlignment::Stretch);

    auto bar = LFBox::create();
    bar->setWidth(m_theme.quoteBarWidth);
    bar->setMinHeight(1.0f);
    bar->setBackgroundColor(m_theme.quoteBarColor);
    row->addChild(bar);

    auto content = LFLinear::createVertical();
    content->setFlexGrow(1.0f);
    content->setFlexShrink(1.0f);
    content->wrapContentHeight();
    content->setPadding(YGEdgeAll, m_theme.quotePadding);
    content->setBackgroundColor(m_theme.quoteBackgroundColor);
    for (const LFMarkdownBlock& child : block.children) {
        renderBlock(child, content);
    }
    row->addChild(content);

    appendBlock(parent, row, m_theme.blockSpacing);
}

void LFMarkdownRenderer::renderList(
        const LFMarkdownBlock& block,
        const std::shared_ptr<LFLinear>& parent,
        float bottomSpacing,
        bool nested) const {
    auto list = LFLinear::createVertical();
    list->matchParentWidth();
    list->wrapContentHeight();
    list->setSpacing(m_theme.listItemSpacing);
    if (!nested && !parent->getChildren().empty()) {
        list->setMargin(YGEdgeTop, m_theme.listBlockSpacing);
    }

    for (size_t index = 0; index < block.children.size(); ++index) {
        const LFMarkdownBlock& item = block.children[index];
        const bool simpleItem =
            item.children.size() == 1 &&
            item.children.front().type == LFMarkdownBlockType::Paragraph;

        auto row = LFLinear::createHorizontal();
        row->matchParentWidth();
        row->wrapContentHeight();
        row->setGravity(
            LFAlignment::Start,
            simpleItem ? LFAlignment::Center : LFAlignment::Start
        );
        row->setSpacing(m_theme.listMarkerGap);

        auto marker = std::make_shared<LFText>();
        marker->setWidth(m_theme.listMarkerWidth);
        marker->wrapContentHeight();
        marker->setFontFamily(m_theme.fontFamily);
        marker->setFontSize(m_theme.bodyFontSize);
        marker->setLineHeight(m_theme.lineHeight);
        marker->setTextColor(m_theme.textColor);
        if (block.type == LFMarkdownBlockType::OrderedList) {
            marker->setTextHAlign(LFTextHAlign::Right);
            marker->setText(
                std::to_string(block.start + static_cast<int>(index)) + "."
            );
        } else {
            marker->setTextHAlign(LFTextHAlign::Center);
            marker->setText("\u2022");
            marker->setTranslate(0.0f, m_theme.bodyFontSize * 0.13f);
        }
        row->addChild(marker);

        auto content = LFLinear::createVertical();
        content->setFlexGrow(1.0f);
        content->setFlexShrink(1.0f);
        content->wrapContentHeight();
        for (const LFMarkdownBlock& child : item.children) {
            if (child.type == LFMarkdownBlockType::Paragraph) {
                renderParagraph(child, content, 0.0f);
            } else if (child.type == LFMarkdownBlockType::UnorderedList ||
                       child.type == LFMarkdownBlockType::OrderedList) {
                renderList(child, content, m_theme.listItemSpacing, true);
            } else {
                renderBlock(child, content);
            }
        }
        row->addChild(content);
        list->addChild(row);
    }

    appendBlock(parent, list, bottomSpacing);
}

void LFMarkdownRenderer::renderCodeBlock(
        const LFMarkdownBlock& block,
        const std::shared_ptr<LFLinear>& parent) const {
    auto box = LFBox::create();
    box->matchParentWidth();
    box->wrapContentHeight();
    box->setPadding(YGEdgeAll, 12.0f);
    box->setBorderRadius(8.0f);
    box->setBackgroundColor(m_theme.codeBlockBackgroundColor);

    auto code = std::make_shared<LFText>();
    code->matchParentWidth();
    code->wrapContentHeight();
    code->setFontFamily(m_theme.codeFontFamily);
    code->setFontSize(m_theme.codeFontSize);
    code->setLineHeight(1.45f);
    code->setTextColor(m_theme.codeTextColor);
    code->setText(block.literal);
    box->addChild(code);
    appendBlock(parent, box, m_theme.blockSpacing);
}

void LFMarkdownRenderer::renderThematicBreak(
        const std::shared_ptr<LFLinear>& parent) const {
    auto divider = LFBox::create();
    divider->matchParentWidth();
    divider->setHeight(1.0f);
    divider->setBackgroundColor(m_theme.dividerColor);
    divider->setMargin(YGEdgeVertical, 6.0f);
    appendBlock(parent, divider, m_theme.blockSpacing);
}

void LFMarkdownRenderer::renderImageBlock(
        const LFMarkdownInline& imageValue,
        const std::shared_ptr<LFLinear>& parent) const {
    auto image = std::make_shared<LFImage>();
    image->setAlignSelf(YGAlignFlexStart);
    image->setMaxWidth(m_theme.imageMaxWidth);
    image->setMaxHeight(m_theme.imageMaxHeight);
    image->setFit(LFImageFit::Contain);
    image->setBorderRadius(6.0f);
    image->setSrc(imageValue.destination);
    appendBlock(parent, image, m_theme.blockSpacing);
}

std::shared_ptr<LFNode> LFMarkdownRenderer::renderInlineFlow(
        const std::vector<LFMarkdownInline>& inlines,
        float fontSize,
        uint32_t textColor,
        LFMarkdownTextAlign align) const {
    auto flow = LFLinear::createVertical();
    flow->matchParentWidth();
    flow->wrapContentHeight();

    std::shared_ptr<LFLinear> row;
    const LFMarkdownView::LinkCallback onLinkTap = m_onLinkTap;

    auto ensureRow = [&]() {
        if (row) return;
        row = LFLinear::createHorizontal();
        row->matchParentWidth();
        row->wrapContentHeight();
        row->setFlexWrap(YGWrapWrap);
        row->setGravity(
            align == LFMarkdownTextAlign::Center
                ? LFAlignment::Center
                : (align == LFMarkdownTextAlign::Right
                    ? LFAlignment::End
                    : LFAlignment::Start),
            LFAlignment::Center
        );
        flow->addChild(row);
    };

    struct Style {
        bool bold = false;
        bool italic = false;
        bool code = false;
        bool link = false;
        std::string destination;
    };

    auto appendSegment = [&](const std::string& value, const Style& style) {
        if (value.empty()) return;
        ensureRow();

        uint32_t color = textColor;
        if (style.link) {
            color = m_theme.linkColor;
        } else if (style.code) {
            color = m_theme.inlineCodeTextColor;
        } else if (style.bold) {
            color = m_theme.boldTextColor;
        } else if (style.italic) {
            color = m_theme.italicTextColor;
        }

        auto text = std::make_shared<LFText>();
        text->setFlexShrink(1.0f);
        text->wrapContentHeight();
        text->setFontFamily(style.code ? m_theme.codeFontFamily : m_theme.fontFamily);
        text->setFontWeight(style.bold ? m_theme.boldWeight : LFFontWeight::Regular);
        text->setItalic(style.italic);
        text->setFontSize(style.code ? std::max(1.0f, fontSize * 0.92f) : fontSize);
        text->setLineHeight(m_theme.lineHeight);
        text->setTextColor(color);
        text->setText(value);

        if (style.link) {
            text->setHoverUnderline(true);
            if (onLinkTap) {
                const std::string destination = style.destination;
                text->setOnTap([onLinkTap, destination](const LFPoint&) {
                    onLinkTap(destination);
                });
            }
        }

        if (style.code) {
            auto codeBox = LFBox::create();
            codeBox->setFlexShrink(1.0f);
            codeBox->wrapContentHeight();
            codeBox->setPadding(YGEdgeHorizontal, 3.0f);
            codeBox->setPadding(YGEdgeVertical, 1.0f);
            codeBox->setMargin(YGEdgeHorizontal, 2.0f);
            codeBox->setBorderRadius(4.0f);
            codeBox->setBackgroundColor(m_theme.inlineCodeBackgroundColor);
            codeBox->addChild(text);
            row->addChild(codeBox);
        } else {
            row->addChild(text);
        }
    };

    auto appendText = [&](const std::string& value, const Style& style) {
        size_t begin = 0;
        while (begin <= value.size()) {
            const size_t end = value.find('\n', begin);
            const size_t length = end == std::string::npos
                ? value.size() - begin
                : end - begin;
            appendSegment(value.substr(begin, length), style);
            if (end == std::string::npos) break;
            row.reset();
            begin = end + 1;
        }
    };

    std::function<void(const std::vector<LFMarkdownInline>&, const Style&)> visit;
    visit = [&](const std::vector<LFMarkdownInline>& values, const Style& style) {
        for (const LFMarkdownInline& value : values) {
            Style childStyle = style;
            switch (value.type) {
                case LFMarkdownInlineType::Text:
                    appendText(value.text, childStyle);
                    break;
                case LFMarkdownInlineType::Bold:
                    childStyle.bold = true;
                    visit(value.children, childStyle);
                    break;
                case LFMarkdownInlineType::Italic:
                    childStyle.italic = true;
                    visit(value.children, childStyle);
                    break;
                case LFMarkdownInlineType::Code:
                    childStyle.code = true;
                    appendText(value.text, childStyle);
                    break;
                case LFMarkdownInlineType::Link:
                    childStyle.link = true;
                    childStyle.destination = value.destination;
                    visit(value.children, childStyle);
                    break;
                case LFMarkdownInlineType::Image: {
                    ensureRow();
                    auto image = std::make_shared<LFImage>();
                    image->setWidth(m_theme.inlineImageWidth);
                    image->setHeight(m_theme.inlineImageHeight);
                    image->setFit(LFImageFit::Contain);
                    image->setBorderRadius(6.0f);
                    image->setBackgroundColor(m_theme.inlineCodeBackgroundColor);
                    image->setSrc(value.destination);
                    row->addChild(image);
                    break;
                }
            }
        }
    };

    visit(inlines, Style{});
    return flow;
}

std::string LFMarkdownRenderer::flattenInline(
        const std::vector<LFMarkdownInline>& inlines) const {
    std::string result;
    for (const LFMarkdownInline& value : inlines) {
        switch (value.type) {
            case LFMarkdownInlineType::Text:
            case LFMarkdownInlineType::Code:
                result += value.text;
                break;
            case LFMarkdownInlineType::Image:
                if (!result.empty()) result.push_back(' ');
                result += flattenInline(value.children);
                break;
            case LFMarkdownInlineType::Bold:
            case LFMarkdownInlineType::Italic:
            case LFMarkdownInlineType::Link:
                result += flattenInline(value.children);
                break;
        }
    }
    return result;
}
