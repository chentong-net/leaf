//
// Platform-independent Markdown document model.
//

#ifndef LEAF_LFMARKDOWNDOCUMENT_H
#define LEAF_LFMARKDOWNDOCUMENT_H

#include <string>
#include <vector>

enum class LFMarkdownBlockType {
    Paragraph,
    Heading,
    Quote,
    UnorderedList,
    OrderedList,
    ListItem,
    CodeBlock,
    ThematicBreak,
    Table
};

enum class LFMarkdownInlineType {
    Text,
    Bold,
    Italic,
    Code,
    Link,
    Image
};

enum class LFMarkdownTextAlign {
    Left,
    Center,
    Right
};

struct LFMarkdownInline {
    LFMarkdownInlineType type = LFMarkdownInlineType::Text;
    std::string text;
    std::string destination;
    std::string title;
    std::vector<LFMarkdownInline> children;
};

struct LFMarkdownTableCell {
    std::vector<LFMarkdownInline> inlines;
};

struct LFMarkdownTableRow {
    std::vector<LFMarkdownTableCell> cells;
};

struct LFMarkdownBlock {
    LFMarkdownBlockType type = LFMarkdownBlockType::Paragraph;
    std::vector<LFMarkdownInline> inlines;
    std::vector<LFMarkdownBlock> children;
    std::vector<LFMarkdownTableRow> rows;
    std::vector<LFMarkdownTextAlign> columnAlignments;
    std::string literal;
    std::string language;
    int level = 0;
    int start = 1;
};

struct LFMarkdownDocument {
    std::vector<LFMarkdownBlock> blocks;

    bool empty() const { return blocks.empty(); }
};

#endif // LEAF_LFMARKDOWNDOCUMENT_H
