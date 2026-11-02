#include "LFMarkdownParser.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct ListMarker {
    bool ordered = false;
    int start = 1;
    size_t indent = 0;
    size_t contentIndent = 0;
    std::string content;
};

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string current;
    for (size_t index = 0; index < text.size(); ++index) {
        const char value = text[index];
        if (value == '\r') {
            if (index + 1 < text.size() && text[index + 1] == '\n') {
                ++index;
            }
            lines.push_back(std::move(current));
            current.clear();
        } else if (value == '\n') {
            lines.push_back(std::move(current));
            current.clear();
        } else {
            current.push_back(value);
        }
    }
    if (!current.empty() || text.empty() || text.back() == '\n' || text.back() == '\r') {
        lines.push_back(std::move(current));
    }
    return lines;
}

bool isBlank(std::string_view line) {
    return std::all_of(line.begin(), line.end(), [](char value) {
        return value == ' ' || value == '\t';
    });
}

size_t leadingSpaces(std::string_view line) {
    size_t count = 0;
    while (count < line.size() && line[count] == ' ') ++count;
    return count;
}

size_t indentation(std::string_view line) {
    size_t columns = 0;
    for (char value : line) {
        if (value == ' ') {
            ++columns;
        } else if (value == '\t') {
            columns += 4;
        } else {
            break;
        }
    }
    return columns;
}

std::string trimLeft(std::string_view text) {
    size_t begin = 0;
    while (begin < text.size() && (text[begin] == ' ' || text[begin] == '\t')) {
        ++begin;
    }
    return std::string(text.substr(begin));
}

std::string trimRight(std::string_view text) {
    size_t end = text.size();
    while (end > 0 && (text[end - 1] == ' ' || text[end - 1] == '\t')) {
        --end;
    }
    return std::string(text.substr(0, end));
}

std::string trim(std::string_view text) {
    return trimRight(trimLeft(text));
}

std::string removeIndent(std::string_view line, size_t columns) {
    size_t index = 0;
    size_t consumed = 0;
    while (index < line.size() && consumed < columns &&
           (line[index] == ' ' || line[index] == '\t')) {
        consumed += line[index] == '\t' ? 4 : 1;
        ++index;
    }
    return std::string(line.substr(index));
}

bool hasQuotePrefix(std::string_view line) {
    const size_t spaces = leadingSpaces(line);
    return spaces <= 3 && spaces < line.size() && line[spaces] == '>';
}

std::string removeQuotePrefix(std::string_view line) {
    size_t index = leadingSpaces(line) + 1;
    if (index < line.size() && line[index] == ' ') ++index;
    return std::string(line.substr(index));
}

ListMarker parseListMarker(std::string_view line) {
    ListMarker marker;
    size_t index = 0;
    size_t columns = 0;
    while (index < line.size() && (line[index] == ' ' || line[index] == '\t')) {
        columns += line[index] == '\t' ? 4 : 1;
        ++index;
    }
    marker.indent = columns;
    if (index >= line.size()) return {};

    if (line[index] == '-' || line[index] == '+' || line[index] == '*') {
        ++index;
    } else if (std::isdigit(static_cast<unsigned char>(line[index]))) {
        marker.ordered = true;
        int value = 0;
        while (index < line.size() &&
               std::isdigit(static_cast<unsigned char>(line[index]))) {
            if (value <= 999999) value = value * 10 + (line[index] - '0');
            ++index;
        }
        if (index >= line.size() || (line[index] != '.' && line[index] != ')')) {
            return {};
        }
        marker.start = std::max(1, value);
        ++index;
    } else {
        return {};
    }

    if (index < line.size() && line[index] != ' ' && line[index] != '\t') {
        return {};
    }

    size_t padding = 0;
    const size_t contentStart = index;
    while (index < line.size() && (line[index] == ' ' || line[index] == '\t')) {
        padding += line[index] == '\t' ? 4 : 1;
        ++index;
    }
    if (padding > 4) {
        index = contentStart + 1;
        padding = 1;
    }
    marker.contentIndent = columns + (marker.ordered ? 2 : 1) +
                           std::max<size_t>(1, padding);
    marker.content = std::string(line.substr(index));
    return marker;
}

bool hasListMarker(std::string_view line) {
    return parseListMarker(line).contentIndent > 0;
}

bool isThematicBreak(std::string_view line) {
    const std::string value = trim(line);
    if (value.size() < 3) return false;

    char marker = '\0';
    int count = 0;
    for (char character : value) {
        if (character == ' ' || character == '\t') continue;
        if (character != '-' && character != '*' && character != '_') return false;
        if (marker == '\0') {
            marker = character;
        } else if (marker != character) {
            return false;
        }
        ++count;
    }
    return count >= 3;
}

bool parseAtxHeading(std::string_view line, int& level, std::string& content) {
    const size_t spaces = leadingSpaces(line);
    if (spaces > 3 || spaces >= line.size() || line[spaces] != '#') return false;

    size_t index = spaces;
    while (index < line.size() && line[index] == '#' && index - spaces < 6) {
        ++index;
    }
    level = static_cast<int>(index - spaces);
    if (level == 0 || (index < line.size() && line[index] != ' ' && line[index] != '\t')) {
        return false;
    }
    content = trim(std::string_view(line).substr(index));
    size_t trailing = content.size();
    while (trailing > 0 && content[trailing - 1] == '#') --trailing;
    if (trailing < content.size() &&
        (trailing == 0 || content[trailing - 1] == ' ' || content[trailing - 1] == '\t')) {
        content = trimRight(std::string_view(content).substr(0, trailing));
    }
    return true;
}

bool parseSetextUnderline(std::string_view line, int& level) {
    const std::string value = trim(line);
    if (value.empty()) return false;
    const char marker = value.front();
    if (marker != '=' && marker != '-') return false;
    if (!std::all_of(value.begin(), value.end(), [marker](char character) {
            return character == marker || character == ' ' || character == '\t';
        })) {
        return false;
    }
    level = marker == '=' ? 1 : 2;
    return true;
}

bool parseFence(std::string_view line, char& marker, size_t& count, std::string& language) {
    const size_t spaces = leadingSpaces(line);
    if (spaces > 3 || spaces >= line.size()) return false;
    const char value = line[spaces];
    if (value != '`' && value != '~') return false;

    size_t index = spaces;
    while (index < line.size() && line[index] == value) ++index;
    count = index - spaces;
    if (count < 3) return false;

    marker = value;
    language = trimLeft(line.substr(index));
    const size_t separator = language.find_first_of(" \t");
    if (separator != std::string::npos) language.resize(separator);
    return true;
}

bool isFenceClose(std::string_view line, char marker, size_t count) {
    const size_t spaces = leadingSpaces(line);
    if (spaces > 3 || spaces >= line.size() || line[spaces] != marker) {
        return false;
    }
    size_t index = spaces;
    while (index < line.size() && line[index] == marker) ++index;
    return index - spaces >= count && isBlank(line.substr(index));
}

std::vector<LFMarkdownInline> parseInline(std::string_view text) {
    LFMarkdownInline value;
    value.type = LFMarkdownInlineType::Text;
    value.text = std::string(text);
    return {std::move(value)};
}

std::string joinParagraph(const std::vector<std::string>& lines,
                          size_t begin,
                          size_t end) {
    std::string result;
    for (size_t index = begin; index < end; ++index) {
        std::string current = trimRight(lines[index]);
        const bool hardBreak =
            (current.size() >= 2 &&
             current[current.size() - 1] == ' ' &&
             current[current.size() - 2] == ' ') ||
            (!current.empty() && current.back() == '\\');
        if (hardBreak) {
            while (!current.empty() && current.back() == ' ') current.pop_back();
            if (!current.empty() && current.back() == '\\') current.pop_back();
        }
        if (!result.empty() && result.back() != '\n') result.push_back(' ');
        result += current;
        if (hardBreak && index + 1 < end) result.push_back('\n');
    }
    return result;
}

bool startsBlock(std::string_view line) {
    int level = 0;
    std::string content;
    if (parseAtxHeading(line, level, content)) return true;
    if (isThematicBreak(line)) return true;
    if (hasQuotePrefix(line)) return true;
    if (hasListMarker(line)) return true;

    char marker = '\0';
    size_t count = 0;
    std::string language;
    return parseFence(line, marker, count, language);
}

std::vector<LFMarkdownBlock> parseBlocks(const std::vector<std::string>& lines,
                                         size_t begin,
                                         size_t end,
                                         int depth);

LFMarkdownBlock parseQuote(const std::vector<std::string>& lines,
                           size_t& index,
                           size_t end,
                           int depth) {
    std::vector<std::string> quoteLines;
    size_t cursor = index;
    while (cursor < end) {
        if (hasQuotePrefix(lines[cursor])) {
            quoteLines.push_back(removeQuotePrefix(lines[cursor]));
            ++cursor;
        } else if (isBlank(lines[cursor]) &&
                   cursor + 1 < end &&
                   hasQuotePrefix(lines[cursor + 1])) {
            quoteLines.emplace_back();
            ++cursor;
        } else {
            break;
        }
    }

    LFMarkdownBlock quote;
    quote.type = LFMarkdownBlockType::Quote;
    quote.children = parseBlocks(quoteLines, 0, quoteLines.size(), depth + 1);
    index = cursor;
    return quote;
}

LFMarkdownBlock parseList(const std::vector<std::string>& lines,
                          size_t& index,
                          size_t end,
                          const ListMarker& first,
                          int depth) {
    LFMarkdownBlock list;
    list.type = first.ordered
        ? LFMarkdownBlockType::OrderedList
        : LFMarkdownBlockType::UnorderedList;
    list.start = first.start;

    size_t cursor = index;
    while (cursor < end) {
        const ListMarker marker = parseListMarker(lines[cursor]);
        if (marker.contentIndent == 0 ||
            marker.ordered != first.ordered ||
            marker.indent != first.indent) {
            break;
        }

        std::vector<std::string> itemLines;
        itemLines.push_back(marker.content);
        ++cursor;

        while (cursor < end) {
            if (isBlank(lines[cursor])) {
                size_t next = cursor + 1;
                while (next < end && isBlank(lines[next])) ++next;
                if (next >= end) {
                    cursor = next;
                    break;
                }
                const ListMarker nextMarker = parseListMarker(lines[next]);
                if (nextMarker.contentIndent > 0 && nextMarker.indent == first.indent) {
                    cursor = next;
                    break;
                }
                itemLines.emplace_back();
                ++cursor;
                continue;
            }

            const ListMarker nextMarker = parseListMarker(lines[cursor]);
            if (nextMarker.contentIndent > 0 && nextMarker.indent == first.indent) {
                break;
            }
            if (indentation(lines[cursor]) <= first.indent) break;
            itemLines.push_back(removeIndent(lines[cursor], marker.contentIndent));
            ++cursor;
        }

        while (!itemLines.empty() && isBlank(itemLines.back())) itemLines.pop_back();

        LFMarkdownBlock item;
        item.type = LFMarkdownBlockType::ListItem;
        if (!itemLines.empty()) {
            item.children = parseBlocks(itemLines, 0, itemLines.size(), depth + 1);
        }
        list.children.push_back(std::move(item));
    }

    index = cursor;
    return list;
}

std::vector<LFMarkdownBlock> parseBlocks(const std::vector<std::string>& lines,
                                         size_t begin,
                                         size_t end,
                                         int depth) {
    if (depth > 64) return {};

    std::vector<LFMarkdownBlock> blocks;
    size_t index = begin;
    while (index < end) {
        if (isBlank(lines[index])) {
            ++index;
            continue;
        }

        char fenceMarker = '\0';
        size_t fenceCount = 0;
        std::string language;
        if (parseFence(lines[index], fenceMarker, fenceCount, language)) {
            LFMarkdownBlock code;
            code.type = LFMarkdownBlockType::CodeBlock;
            code.language = std::move(language);
            ++index;
            while (index < end && !isFenceClose(lines[index], fenceMarker, fenceCount)) {
                if (!code.literal.empty()) code.literal.push_back('\n');
                code.literal += lines[index];
                ++index;
            }
            if (index < end) ++index;
            blocks.push_back(std::move(code));
            continue;
        }

        int headingLevel = 0;
        std::string headingText;
        if (parseAtxHeading(lines[index], headingLevel, headingText)) {
            LFMarkdownBlock heading;
            heading.type = LFMarkdownBlockType::Heading;
            heading.level = headingLevel;
            heading.inlines = parseInline(headingText);
            blocks.push_back(std::move(heading));
            ++index;
            continue;
        }

        if (hasQuotePrefix(lines[index])) {
            blocks.push_back(parseQuote(lines, index, end, depth));
            continue;
        }

        if (isThematicBreak(lines[index])) {
            LFMarkdownBlock divider;
            divider.type = LFMarkdownBlockType::ThematicBreak;
            blocks.push_back(std::move(divider));
            ++index;
            continue;
        }

        const ListMarker listMarker = parseListMarker(lines[index]);
        if (listMarker.contentIndent > 0) {
            blocks.push_back(parseList(lines, index, end, listMarker, depth));
            continue;
        }

        if (index + 1 < end) {
            int setextLevel = 0;
            if (parseSetextUnderline(lines[index + 1], setextLevel)) {
                LFMarkdownBlock heading;
                heading.type = LFMarkdownBlockType::Heading;
                heading.level = setextLevel;
                heading.inlines = parseInline(trim(lines[index]));
                blocks.push_back(std::move(heading));
                index += 2;
                continue;
            }
        }

        const size_t paragraphBegin = index;
        ++index;
        while (index < end && !isBlank(lines[index]) && !startsBlock(lines[index])) {
            int setextLevel = 0;
            if (parseSetextUnderline(lines[index], setextLevel)) break;
            ++index;
        }

        LFMarkdownBlock paragraph;
        paragraph.type = LFMarkdownBlockType::Paragraph;
        paragraph.inlines = parseInline(joinParagraph(lines, paragraphBegin, index));
        blocks.push_back(std::move(paragraph));
    }
    return blocks;
}

} // namespace

LFMarkdownDocument LFMarkdownParser::parse(const std::string& markdown) {
    LFMarkdownDocument document;
    const std::vector<std::string> lines = splitLines(markdown);
    document.blocks = parseBlocks(lines, 0, lines.size(), 0);
    return document;
}
