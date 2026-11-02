#include "LFMarkdownParser.h"

#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "FAILED: " << message << '\n';
    ++failures;
}

void testBlocks() {
    const LFMarkdownDocument document = LFMarkdownParser::parse(
        "# Heading\n\n"
        "Paragraph\n\n"
        "> outer\n"
        ">> inner\n\n"
        "1. ordered\n"
        "2. second\n"
        "   - nested\n\n"
        "```cpp\nint value = 1;\n```\n\n"
        "---\n"
    );

    bool hasHeading = false;
    bool hasParagraph = false;
    bool hasQuote = false;
    bool hasOrdered = false;
    bool hasCode = false;
    bool hasDivider = false;
    for (const LFMarkdownBlock& block : document.blocks) {
        hasHeading |= block.type == LFMarkdownBlockType::Heading;
        hasParagraph |= block.type == LFMarkdownBlockType::Paragraph;
        hasQuote |= block.type == LFMarkdownBlockType::Quote;
        hasOrdered |= block.type == LFMarkdownBlockType::OrderedList;
        hasCode |= block.type == LFMarkdownBlockType::CodeBlock;
        hasDivider |= block.type == LFMarkdownBlockType::ThematicBreak;
    }

    expect(hasHeading, "heading block");
    expect(hasParagraph, "paragraph block");
    expect(hasQuote, "quote block");
    expect(hasOrdered, "ordered list block");
    expect(hasCode, "code block");
    expect(hasDivider, "divider block");
}

void testInlineStyles() {
    const LFMarkdownDocument document = LFMarkdownParser::parse(
        "**bold** *italic* `code` [link](https://example.com) "
        "![image](image.png \"title\")\n"
    );
    expect(document.blocks.size() == 1, "inline paragraph");
    if (document.blocks.empty()) return;

    const auto& values = document.blocks.front().inlines;
    bool hasBold = false;
    bool hasItalic = false;
    bool hasCode = false;
    bool hasLink = false;
    bool hasImage = false;
    for (const LFMarkdownInline& value : values) {
        hasBold |= value.type == LFMarkdownInlineType::Bold;
        hasItalic |= value.type == LFMarkdownInlineType::Italic;
        hasCode |= value.type == LFMarkdownInlineType::Code;
        hasLink |= value.type == LFMarkdownInlineType::Link;
        hasImage |= value.type == LFMarkdownInlineType::Image;
    }
    expect(hasBold, "bold span");
    expect(hasItalic, "italic span");
    expect(hasCode, "code span");
    expect(hasLink, "link span");
    expect(hasImage, "image span");
}

void testTable() {
    const LFMarkdownDocument document = LFMarkdownParser::parse(
        "| Name | Value |\n"
        "| :--- | ---: |\n"
        "| Alpha | 1 |\n"
        "| Beta | 2 |\n"
    );
    expect(document.blocks.size() == 1, "table block");
    if (document.blocks.empty()) return;

    const LFMarkdownBlock& table = document.blocks.front();
    expect(table.type == LFMarkdownBlockType::Table, "table type");
    expect(table.rows.size() == 3, "table row count");
    expect(table.columnAlignments.size() == 2, "table alignment count");
    if (table.columnAlignments.size() == 2) {
        expect(table.columnAlignments[0] == LFMarkdownTextAlign::Left,
               "left column alignment");
        expect(table.columnAlignments[1] == LFMarkdownTextAlign::Right,
               "right column alignment");
    }
}

void testNestedQuote() {
    const LFMarkdownDocument document = LFMarkdownParser::parse(
        "> first\n"
        ">> second\n"
        ">>> third\n"
    );
    expect(document.blocks.size() == 1, "nested quote root");
    if (document.blocks.empty()) return;

    const LFMarkdownBlock* quote = &document.blocks.front();
    int depth = 0;
    while (quote && quote->type == LFMarkdownBlockType::Quote) {
        ++depth;
        quote = quote->children.empty() ? nullptr : &quote->children.back();
    }
    expect(depth == 3, "nested quote depth");
}

void testHtmlRemainsText() {
    const LFMarkdownDocument document = LFMarkdownParser::parse(
        "<strong>raw html</strong>\n"
    );
    expect(document.blocks.size() == 1, "html paragraph");
    if (document.blocks.empty()) return;
    expect(document.blocks.front().inlines.size() == 1, "html remains text");
    if (!document.blocks.front().inlines.empty()) {
        expect(document.blocks.front().inlines.front().type ==
                   LFMarkdownInlineType::Text,
               "html inline type");
    }
}

void testSetextHeadings() {
    const LFMarkdownDocument document = LFMarkdownParser::parse(
        "First level\n"
        "===========\n\n"
        "Second level\n"
        "------------\n"
    );
    expect(document.blocks.size() == 2, "setext heading count");
    if (document.blocks.size() != 2) return;

    expect(document.blocks[0].type == LFMarkdownBlockType::Heading,
           "setext first block type");
    expect(document.blocks[0].level == 1, "setext first heading level");
    expect(document.blocks[1].type == LFMarkdownBlockType::Heading,
           "setext second block type");
    expect(document.blocks[1].level == 2, "setext second heading level");
}

void testHardBreaks() {
    const LFMarkdownDocument document = LFMarkdownParser::parse(
        "backslash\\\n"
        "break\n\n"
        "spaces  \n"
        "break\n"
    );
    expect(document.blocks.size() == 2, "hard break paragraph count");
    if (document.blocks.size() != 2) return;

    expect(document.blocks[0].inlines.size() == 1,
           "backslash hard break inline count");
    expect(document.blocks[1].inlines.size() == 1,
           "space hard break inline count");
    if (!document.blocks[0].inlines.empty()) {
        expect(document.blocks[0].inlines[0].text == "backslash\nbreak",
               "backslash hard break content");
    }
    if (!document.blocks[1].inlines.empty()) {
        expect(document.blocks[1].inlines[0].text == "spaces\nbreak",
               "space hard break content");
    }
}

void testListStartAndChildren() {
    const LFMarkdownDocument document = LFMarkdownParser::parse(
        "4. four\n"
        "5. five\n"
        "   - nested\n"
    );
    expect(document.blocks.size() == 1, "ordered list block count");
    if (document.blocks.size() != 1) return;

    const LFMarkdownBlock& list = document.blocks.front();
    expect(list.type == LFMarkdownBlockType::OrderedList,
           "ordered list type");
    expect(list.start == 4, "ordered list start");
    expect(list.children.size() == 2, "ordered list item count");
    if (list.children.size() < 2) return;
    expect(list.children[1].children.size() == 2,
           "nested list item block count");
}

void testFenceVariants() {
    const LFMarkdownDocument document = LFMarkdownParser::parse(
        "```cpp\n"
        "int value = 1;\n"
        "```\n\n"
        "~~~text\n"
        "    indented\n"
        "~~~\n"
    );
    expect(document.blocks.size() == 2, "fence block count");
    if (document.blocks.size() != 2) return;

    expect(document.blocks[0].type == LFMarkdownBlockType::CodeBlock,
           "backtick fence type");
    expect(document.blocks[0].language == "cpp",
           "backtick fence language");
    expect(document.blocks[0].literal == "int value = 1;",
           "backtick fence content");
    expect(document.blocks[1].type == LFMarkdownBlockType::CodeBlock,
           "tilde fence type");
    expect(document.blocks[1].language == "text",
           "tilde fence language");
    expect(document.blocks[1].literal == "    indented",
           "tilde fence indentation");
}

void testTableAlignments() {
    const LFMarkdownDocument document = LFMarkdownParser::parse(
        "| Left | Center | Right |\n"
        "| :--- | :---: | ---: |\n"
        "| A | B | C |\n"
    );
    expect(document.blocks.size() == 1, "alignment table block count");
    if (document.blocks.size() != 1) return;

    const LFMarkdownBlock& table = document.blocks.front();
    expect(table.columnAlignments.size() == 3,
           "alignment table column count");
    if (table.columnAlignments.size() != 3) return;
    expect(table.columnAlignments[0] == LFMarkdownTextAlign::Left,
           "table left alignment");
    expect(table.columnAlignments[1] == LFMarkdownTextAlign::Center,
           "table center alignment");
    expect(table.columnAlignments[2] == LFMarkdownTextAlign::Right,
           "table right alignment");
}

void testEscapesAndUnclosedMarkers() {
    const LFMarkdownDocument document = LFMarkdownParser::parse(
        "\\*plain\\* \\[text\\] \\`code\\`\n\n"
        "**unclosed and `unclosed\n"
    );
    expect(document.blocks.size() == 2,
           "escape and unclosed paragraph count");
    if (document.blocks.size() != 2) return;

    expect(document.blocks[0].inlines.size() == 1,
           "escaped syntax remains text");
    if (!document.blocks[0].inlines.empty()) {
        expect(document.blocks[0].inlines[0].type ==
                   LFMarkdownInlineType::Text,
               "escaped syntax inline type");
    }
    expect(document.blocks[1].inlines.size() == 1,
           "unclosed syntax remains text");
    if (!document.blocks[1].inlines.empty()) {
        expect(document.blocks[1].inlines[0].type ==
                   LFMarkdownInlineType::Text,
               "unclosed syntax inline type");
    }
}

} // namespace

int main() {
    testBlocks();
    testInlineStyles();
    testTable();
    testNestedQuote();
    testHtmlRemainsText();
    testSetextHeadings();
    testHardBreaks();
    testListStartAndChildren();
    testFenceVariants();
    testTableAlignments();
    testEscapesAndUnclosedMarkers();

    if (failures != 0) {
        std::cerr << failures << " Markdown test(s) failed\n";
        return 1;
    }
    std::cout << "All Markdown tests passed\n";
    return 0;
}
