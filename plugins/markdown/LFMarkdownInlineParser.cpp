#include "LFMarkdownInlineParser.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr int kMaximumDepth = 32;

bool isEscapable(char value) {
    switch (value) {
        case '\\':
        case '`':
        case '*':
        case '_':
        case '{':
        case '}':
        case '[':
        case ']':
        case '(':
        case ')':
        case '#':
        case '+':
        case '-':
        case '.':
        case '!':
        case '|':
        case '>':
        case '<':
            return true;
        default:
            return false;
    }
}

bool isWhitespace(char value) {
    return std::isspace(static_cast<unsigned char>(value)) != 0;
}

bool isWordCharacter(char value) {
    return std::isalnum(static_cast<unsigned char>(value)) != 0;
}

void appendText(std::vector<LFMarkdownInline>& output, std::string text) {
    if (text.empty()) return;
    if (!output.empty() && output.back().type == LFMarkdownInlineType::Text) {
        output.back().text += text;
        return;
    }

    LFMarkdownInline value;
    value.type = LFMarkdownInlineType::Text;
    value.text = std::move(text);
    output.push_back(std::move(value));
}

size_t markerRunLength(std::string_view text, size_t begin, char marker) {
    size_t end = begin;
    while (end < text.size() && text[end] == marker) ++end;
    return end - begin;
}

bool isEscaped(std::string_view text, size_t index) {
    size_t slashes = 0;
    while (index > 0 && text[index - 1] == '\\') {
        ++slashes;
        --index;
    }
    return slashes % 2 == 1;
}

size_t findMarkerRun(std::string_view text,
                     size_t begin,
                     char marker,
                     size_t count) {
    for (size_t index = begin; index + count <= text.size(); ++index) {
        if (isEscaped(text, index)) continue;
        bool matches = true;
        for (size_t offset = 0; offset < count; ++offset) {
            if (text[index + offset] != marker) {
                matches = false;
                break;
            }
        }
        if (matches) return index;
    }
    return std::string_view::npos;
}

size_t findClosingBracket(std::string_view text, size_t openBracket) {
    int depth = 1;
    for (size_t index = openBracket + 1; index < text.size(); ++index) {
        if (text[index] == '\\' && index + 1 < text.size()) {
            ++index;
            continue;
        }
        if (text[index] == '[') {
            ++depth;
        } else if (text[index] == ']') {
            --depth;
            if (depth == 0) return index;
        }
    }
    return std::string_view::npos;
}

std::string unescape(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (size_t index = 0; index < text.size(); ++index) {
        if (text[index] == '\\' && index + 1 < text.size() &&
            isEscapable(text[index + 1])) {
            result.push_back(text[++index]);
        } else {
            result.push_back(text[index]);
        }
    }
    return result;
}

bool parseLinkTarget(std::string_view text,
                     size_t openParen,
                     size_t& end,
                     std::string& destination,
                     std::string& title) {
    size_t index = openParen + 1;
    while (index < text.size() && (text[index] == ' ' || text[index] == '\t')) {
        ++index;
    }

    const size_t destinationBegin = index;
    if (index < text.size() && text[index] == '<') {
        ++index;
        const size_t valueBegin = index;
        while (index < text.size() && text[index] != '>') {
            if (text[index] == '\\' && index + 1 < text.size()) ++index;
            ++index;
        }
        if (index >= text.size()) return false;
        destination = unescape(text.substr(valueBegin, index - valueBegin));
        ++index;
    } else {
        int parenthesisDepth = 0;
        while (index < text.size()) {
            const char value = text[index];
            if (value == '\\' && index + 1 < text.size()) {
                index += 2;
                continue;
            }
            if (value == '(') {
                ++parenthesisDepth;
            } else if (value == ')') {
                if (parenthesisDepth == 0) break;
                --parenthesisDepth;
            } else if ((value == ' ' || value == '\t') && parenthesisDepth == 0) {
                break;
            }
            ++index;
        }
        destination = unescape(text.substr(destinationBegin, index - destinationBegin));
    }

    while (index < text.size() && (text[index] == ' ' || text[index] == '\t')) {
        ++index;
    }

    if (index < text.size() && text[index] != ')') {
        const char opener = text[index];
        char closer = '\0';
        if (opener == '"') closer = '"';
        else if (opener == '\'') closer = '\'';
        else if (opener == '(') closer = ')';
        else return false;

        ++index;
        const size_t titleBegin = index;
        while (index < text.size() && text[index] != closer) {
            if (text[index] == '\\' && index + 1 < text.size()) ++index;
            ++index;
        }
        if (index >= text.size()) return false;
        title = unescape(text.substr(titleBegin, index - titleBegin));
        ++index;
        while (index < text.size() && (text[index] == ' ' || text[index] == '\t')) {
            ++index;
        }
    }

    if (index >= text.size() || text[index] != ')') return false;
    end = index + 1;
    return true;
}

std::string normalizeCodeSpan(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    bool previousSpace = false;
    for (char character : text) {
        const char value = character == '\n' || character == '\r' ? ' ' : character;
        if (value == ' ' && previousSpace) continue;
        result.push_back(value);
        previousSpace = value == ' ';
    }
    if (result.size() >= 2 && result.front() == ' ' && result.back() == ' ' &&
        result.find_first_not_of(' ') != std::string::npos) {
        result = result.substr(1, result.size() - 2);
    }
    return result;
}

std::vector<LFMarkdownInline> parseRange(std::string_view text,
                                         bool allowLinks,
                                         int depth);

bool tryParseLinkOrImage(std::string_view text,
                         size_t& index,
                         bool image,
                         bool allowLinks,
                         std::vector<LFMarkdownInline>& output,
                         int depth) {
    const size_t imagePrefix = image ? 1 : 0;
    const size_t openBracket = index + imagePrefix;
    if (openBracket >= text.size() || text[openBracket] != '[') return false;
    if (!image && !allowLinks) return false;

    const size_t closeBracket = findClosingBracket(text, openBracket);
    if (closeBracket == std::string_view::npos ||
        closeBracket + 1 >= text.size() ||
        text[closeBracket + 1] != '(') {
        return false;
    }

    size_t end = closeBracket + 1;
    std::string destination;
    std::string title;
    if (!parseLinkTarget(text, closeBracket + 1, end, destination, title)) {
        return false;
    }

    LFMarkdownInline value;
    value.type = image ? LFMarkdownInlineType::Image : LFMarkdownInlineType::Link;
    value.destination = std::move(destination);
    value.title = std::move(title);
    value.children = parseRange(
        text.substr(openBracket + 1, closeBracket - openBracket - 1),
        false,
        depth + 1
    );
    output.push_back(std::move(value));
    index = end;
    return true;
}

bool tryParseStyle(std::string_view text,
                   size_t& index,
                   std::vector<LFMarkdownInline>& output,
                   int depth) {
    const char marker = text[index];
    if (marker != '*' && marker != '_') return false;

    const size_t run = markerRunLength(text, index, marker);
    if (run == 0) return false;
    if (marker == '_' && index > 0 && index + 1 < text.size() &&
        isWordCharacter(text[index - 1]) && isWordCharacter(text[index + 1])) {
        return false;
    }

    const size_t candidates[] = {std::min<size_t>(run, 3), 2, 1};
    for (size_t count : candidates) {
        if (count == 0 || count > run) continue;
        const size_t contentBegin = index + count;
        if (contentBegin >= text.size() || isWhitespace(text[contentBegin])) continue;

        const size_t close = findMarkerRun(text, contentBegin, marker, count);
        if (close == std::string_view::npos || close == contentBegin ||
            isWhitespace(text[close - 1])) {
            continue;
        }

        std::vector<LFMarkdownInline> children = parseRange(
            text.substr(contentBegin, close - contentBegin),
            true,
            depth + 1
        );
        if (children.empty()) continue;

        LFMarkdownInline value;
        if (count >= 3) {
            value.type = LFMarkdownInlineType::Bold;
            LFMarkdownInline italic;
            italic.type = LFMarkdownInlineType::Italic;
            italic.children = std::move(children);
            value.children.push_back(std::move(italic));
        } else {
            value.type = count == 2
                ? LFMarkdownInlineType::Bold
                : LFMarkdownInlineType::Italic;
            value.children = std::move(children);
        }
        output.push_back(std::move(value));
        index = close + count;
        return true;
    }
    return false;
}

std::vector<LFMarkdownInline> parseRange(std::string_view text,
                                         bool allowLinks,
                                         int depth) {
    std::vector<LFMarkdownInline> output;
    if (depth > kMaximumDepth) {
        appendText(output, std::string(text));
        return output;
    }

    std::string pending;
    auto flush = [&]() {
        if (pending.empty()) return;
        appendText(output, std::move(pending));
        pending.clear();
    };

    size_t index = 0;
    while (index < text.size()) {
        const char value = text[index];
        if (value == '\\' && index + 1 < text.size() && isEscapable(text[index + 1])) {
            pending.push_back(text[index + 1]);
            index += 2;
            continue;
        }

        if (value == '`') {
            flush();
            const size_t count = markerRunLength(text, index, '`');
            const size_t close = findMarkerRun(text, index + count, '`', count);
            if (close == std::string_view::npos) {
                pending.append(count, '`');
                index += count;
                continue;
            }
            LFMarkdownInline code;
            code.type = LFMarkdownInlineType::Code;
            code.text = normalizeCodeSpan(
                text.substr(index + count, close - index - count)
            );
            output.push_back(std::move(code));
            index = close + count;
            continue;
        }

        if (value == '!' && index + 1 < text.size() && text[index + 1] == '[') {
            flush();
            if (tryParseLinkOrImage(text, index, true, allowLinks, output, depth)) {
                continue;
            }
            pending.push_back(value);
            ++index;
            continue;
        }

        if (value == '[') {
            flush();
            if (tryParseLinkOrImage(text, index, false, allowLinks, output, depth)) {
                continue;
            }
            pending.push_back(value);
            ++index;
            continue;
        }

        if (value == '*' || value == '_') {
            flush();
            if (tryParseStyle(text, index, output, depth)) continue;
            pending.push_back(value);
            ++index;
            continue;
        }

        pending.push_back(value);
        ++index;
    }
    flush();
    return output;
}

} // namespace

std::vector<LFMarkdownInline> LFMarkdownInlineParser::parse(std::string_view text) {
    return parseRange(text, true, 0);
}
