#include "LFMarkdownRenderer.h"

#include "event/LFEvent.h"
#include "view/base/LFImage.h"
#include "view/base/LFText.h"
#include "view/layout/LFBox.h"

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view kIdeographicComma = "\xE3\x80\x81";

size_t visibleCharacters(const std::string& text) {
    size_t count = 0;
    for (unsigned char value : text) {
        if ((value & 0xC0) != 0x80) ++count;
    }
    return count;
}

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

    renderBlocks(document.blocks, root, m_theme.blockSpacing);
    return root;
}

void LFMarkdownRenderer::renderBlocks(
        const std::vector<LFMarkdownBlock>& blocks,
        const std::shared_ptr<LFLinear>& parent,
        float defaultGap) const {
    for (size_t index = 0; index < blocks.size(); ++index) {
        const LFMarkdownBlock* next =
            index + 1 < blocks.size() ? &blocks[index + 1] : nullptr;
        renderBlock(
            blocks[index],
            parent,
            spacingAfterBlock(blocks[index], next, defaultGap)
        );
    }
}

void LFMarkdownRenderer::renderBlock(
        const LFMarkdownBlock& block,
        const std::shared_ptr<LFLinear>& parent,
        float blockGap) const {
    switch (block.type) {
        case LFMarkdownBlockType::Paragraph:
            renderParagraph(block, parent, blockGap);
            break;
        case LFMarkdownBlockType::Heading:
            renderHeading(block, parent);
            break;
        case LFMarkdownBlockType::Quote:
            renderQuote(block, parent, blockGap);
            break;
        case LFMarkdownBlockType::UnorderedList:
        case LFMarkdownBlockType::OrderedList:
            renderList(block, parent, blockGap);
            break;
        case LFMarkdownBlockType::CodeBlock:
            renderCodeBlock(block, parent, blockGap);
            break;
        case LFMarkdownBlockType::ThematicBreak:
            renderThematicBreak(parent, blockGap);
            break;
        case LFMarkdownBlockType::Table:
            renderTable(block, parent, blockGap);
            break;
        case LFMarkdownBlockType::ListItem:
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
        const std::shared_ptr<LFLinear>& parent,
        float blockGap) const {
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
    content->setPadding(YGEdgeLeft, m_theme.quoteHorizontalPadding);
    content->setPadding(YGEdgeRight, m_theme.quoteHorizontalPadding);
    content->setPadding(YGEdgeTop, m_theme.quoteVerticalPadding);
    content->setPadding(YGEdgeBottom, m_theme.quoteVerticalPadding);
    content->setBackgroundColor(m_theme.quoteBackgroundColor);
    renderBlocks(block.children, content, m_theme.quoteBlockSpacing);
    row->addChild(content);

    appendBlock(parent, row, blockGap);
}

void LFMarkdownRenderer::renderList(
        const LFMarkdownBlock& block,
        const std::shared_ptr<LFLinear>& parent,
        float bottomSpacing) const {
    auto list = LFLinear::createVertical();
    list->matchParentWidth();
    list->wrapContentHeight();

    for (size_t index = 0; index < block.children.size(); ++index) {
        const LFMarkdownBlock& item = block.children[index];
        auto row = LFLinear::createHorizontal();
        row->matchParentWidth();
        row->wrapContentHeight();
        row->setGravity(LFAlignment::Start, LFAlignment::Baseline);
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
        }
        row->addChild(marker);

        auto content = LFLinear::createVertical();
        content->setFlexGrow(1.0f);
        content->setFlexShrink(1.0f);
        content->wrapContentHeight();
        renderBlocks(item.children, content, m_theme.listItemSpacing);
        row->addChild(content);
        if (index + 1 < block.children.size()) {
            row->setMargin(YGEdgeBottom, m_theme.listItemSpacing);
        }
        list->addChild(row);
    }

    appendBlock(parent, list, bottomSpacing);
}

float LFMarkdownRenderer::spacingAfterBlock(
        const LFMarkdownBlock& current,
        const LFMarkdownBlock* next,
        float defaultGap) const {
    if (!next) return 0.0f;

    const bool currentIsParagraph =
        current.type == LFMarkdownBlockType::Paragraph;
    const bool nextIsParagraph =
        next->type == LFMarkdownBlockType::Paragraph;
    const bool currentIsList =
        current.type == LFMarkdownBlockType::UnorderedList ||
        current.type == LFMarkdownBlockType::OrderedList;
    const bool nextIsList =
        next->type == LFMarkdownBlockType::UnorderedList ||
        next->type == LFMarkdownBlockType::OrderedList;

    if ((currentIsParagraph && nextIsList) ||
        (currentIsList && nextIsParagraph)) {
        return m_theme.listItemSpacing;
    }
    return defaultGap;
}

void LFMarkdownRenderer::renderCodeBlock(
        const LFMarkdownBlock& block,
        const std::shared_ptr<LFLinear>& parent,
        float blockGap) const {
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
    appendBlock(parent, box, blockGap);
}

void LFMarkdownRenderer::renderThematicBreak(
        const std::shared_ptr<LFLinear>& parent,
        float blockGap) const {
    auto divider = LFBox::create();
    divider->matchParentWidth();
    divider->setHeight(1.0f);
    divider->setBackgroundColor(m_theme.dividerColor);
    divider->setMargin(YGEdgeVertical, 6.0f);
    appendBlock(parent, divider, blockGap);
}

void LFMarkdownRenderer::renderTable(
        const LFMarkdownBlock& block,
        const std::shared_ptr<LFLinear>& parent,
        float blockGap) const {
    if (block.rows.empty()) return;

    size_t columnCount = block.columnAlignments.size();
    for (const LFMarkdownTableRow& row : block.rows) {
        columnCount = std::max(columnCount, row.cells.size());
    }
    if (columnCount == 0) return;

    std::vector<size_t> weights(columnCount, 1);
    for (const LFMarkdownTableRow& row : block.rows) {
        for (size_t column = 0; column < row.cells.size(); ++column) {
            weights[column] = std::max(
                weights[column],
                std::min<size_t>(
                    12,
                    visibleCharacters(flattenInline(row.cells[column].inlines))
                )
            );
        }
    }

    auto table = LFLinear::createVertical();
    table->matchParentWidth();
    table->wrapContentHeight();
    table->setSpacing(1.0f);
    table->setPadding(YGEdgeAll, 1.0f);
    table->setBorderRadius(7.0f);
    table->setMasksToBounds(true);
    table->setBackgroundColor(m_theme.tableBorderColor);

    for (size_t rowIndex = 0; rowIndex < block.rows.size(); ++rowIndex) {
        const LFMarkdownTableRow& tableRow = block.rows[rowIndex];
        auto row = LFLinear::createHorizontal();
        row->matchParentWidth();
        row->wrapContentHeight();
        row->setSpacing(1.0f);
        row->setGravity(LFAlignment::Start, LFAlignment::Stretch);

        for (size_t column = 0; column < columnCount; ++column) {
            auto cell = LFBox::create();
            cell->setFlexGrow(static_cast<float>(weights[column]));
            cell->setFlexShrink(1.0f);
            cell->setFlexBasis(0.0f);
            cell->setMinHeight(m_theme.tableCellMinHeight);
            cell->setPadding(YGEdgeAll, m_theme.tableCellPadding);
            cell->setBackgroundColor(
                rowIndex == 0
                    ? m_theme.tableHeaderBackgroundColor
                    : m_theme.tableBodyBackgroundColor
            );

            auto text = std::make_shared<LFText>();
            text->matchParentWidth();
            text->wrapContentHeight();
            text->setFontFamily(m_theme.fontFamily);
            text->setFontWeight(
                rowIndex == 0 ? LFFontWeight::SemiBold : LFFontWeight::Regular
            );
            text->setFontSize(std::max(1.0f, m_theme.bodyFontSize - 1.0f));
            text->setLineHeight(1.4f);
            text->setTextColor(
                rowIndex == 0 ? m_theme.headingColor : m_theme.textColor
            );

            LFMarkdownTextAlign alignment = LFMarkdownTextAlign::Left;
            if (column < block.columnAlignments.size()) {
                alignment = block.columnAlignments[column];
            }
            text->setTextHAlign(
                alignment == LFMarkdownTextAlign::Center
                    ? LFTextHAlign::Center
                    : (alignment == LFMarkdownTextAlign::Right
                        ? LFTextHAlign::Right
                        : LFTextHAlign::Left)
            );

            std::string value;
            if (column < tableRow.cells.size()) {
                value = flattenInline(tableRow.cells[column].inlines);
            }
            text->setText(value.empty() ? " " : value);
            cell->addChild(text);
            row->addChild(cell);
        }
        table->addChild(row);
    }

    appendBlock(parent, table, blockGap);
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
            LFAlignment::Baseline
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

    auto appendSegment = [&](const std::string& value,
                             const Style& style,
                             bool compressTrailingSpace) {
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
        if (compressTrailingSpace) {
            text->setMargin(
                YGEdgeRight,
                -fontSize * m_theme.cjkPunctuationCompression
            );
        }

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

    auto appendText = [&](const std::string& value,
                          const Style& style,
                          bool splitPunctuation) {
        size_t begin = 0;
        while (begin <= value.size()) {
            const size_t end = value.find('\n', begin);
            const size_t length = end == std::string::npos
                ? value.size() - begin
                : end - begin;
            const std::string segment = value.substr(begin, length);
            if (splitPunctuation) {
                size_t cursor = 0;
                while (cursor < segment.size()) {
                    const size_t comma = segment.find(kIdeographicComma, cursor);
                    if (comma == std::string::npos) {
                        appendSegment(segment.substr(cursor), style, false);
                        break;
                    }
                    appendSegment(segment.substr(cursor, comma - cursor), style, false);
                    appendSegment(
                        std::string(kIdeographicComma),
                        style,
                        true
                    );
                    cursor = comma + kIdeographicComma.size();
                }
            } else {
                appendSegment(segment, style, false);
            }
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
                    appendText(value.text, childStyle, true);
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
                    appendText(value.text, childStyle, false);
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
