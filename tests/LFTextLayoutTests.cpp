#include "text/LFTextLayout.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

int failureCount = 0;

void expect(bool condition, const char* message) {
    if (condition) {
        return;
    }
    std::cerr << "FAILED: " << message << '\n';
    ++failureCount;
}

size_t codepointCount(std::string_view text) {
    size_t count = 0;
    for (unsigned char byte : text) {
        if ((byte & 0xC0) != 0x80) {
            ++count;
        }
    }
    return count;
}

LFTextLineMetrics fixedMeasure(std::string_view text) {
    LFTextLineMetrics metrics;
    metrics.advance = static_cast<float>(codepointCount(text)) * 10.0f;
    metrics.inkMinX = metrics.advance > 0.0f ? -1.0f : 0.0f;
    metrics.inkMaxX = metrics.advance > 0.0f ? metrics.advance - 2.0f : 0.0f;
    return metrics;
}

LFTextLayout buildLayout(std::string_view text, float wrapWidth, int maxLines = 0) {
    LFTextLayoutOptions options;
    options.wrapWidth = wrapWidth;
    options.maxLines = maxLines;
    options.lineAdvance = 20.0f;
    options.naturalLineHeight = 14.0f;
    options.firstBaseline = 11.0f;
    return LFTextLayoutBuilder::build(text, options, fixedMeasure);
}

std::string lineText(std::string_view text, const LFTextLine& line) {
    return std::string(text.substr(line.begin, line.end - line.begin));
}

void testLatinWordWrapping() {
    const std::string text = "alpha beta gamma";
    const LFTextLayout layout = buildLayout(text, 80.0f);

    expect(layout.lines.size() == 3, "Latin words should wrap on spaces");
    if (layout.lines.size() == 3) {
        expect(lineText(text, layout.lines[0]) == "alpha", "first Latin line");
        expect(lineText(text, layout.lines[1]) == "beta", "second Latin line");
        expect(lineText(text, layout.lines[2]) == "gamma", "third Latin line");
        expect(std::fabs(layout.height - 54.0f) < 0.001f,
               "trimmed height keeps inter-line advance only");
        expect(std::fabs(layout.firstBaseline - 11.0f) < 0.001f,
               "layout exposes the first baseline");
    }
}

void testTrailingSpacesAreNotDrawn() {
    const std::string text = "alpha   beta";
    const LFTextLayout layout = buildLayout(text, 80.0f);

    expect(layout.lines.size() == 2, "trailing spaces should not create a line");
    if (layout.lines.size() == 2) {
        expect(lineText(text, layout.lines[0]) == "alpha", "trailing spaces trim");
        expect(lineText(text, layout.lines[1]) == "beta", "next line starts at word");
    }
}

void testCjkPunctuationDoesNotStartLine() {
    const std::string text = "你好，世界";
    const LFTextLayout layout = buildLayout(text, 30.0f);

    expect(layout.lines.size() == 2, "CJK text should wrap near punctuation");
    for (const auto& line : layout.lines) {
        expect(lineText(text, line) != "，世界", "closing punctuation cannot start line");
    }
}

void testEmojiSequenceStaysTogether() {
    const std::string text = "👩‍💻👩‍💻";
    const LFTextLayout layout = buildLayout(text, 30.0f);

    expect(layout.lines.size() == 2, "emoji ZWJ sequences wrap as clusters");
    for (const auto& line : layout.lines) {
        const std::string value = lineText(text, line);
        expect(!value.empty(), "emoji line must not be empty");
        expect(static_cast<unsigned char>(value.front()) != 0xE2 ||
                   value.size() < 3 ||
                   static_cast<unsigned char>(value[2]) != 0x80,
               "emoji line cannot start with ZWJ");
    }
}

void testCombiningMarkStaysWithBase() {
    const std::string text = "e\u0301e\u0301";
    const LFTextLayout layout = buildLayout(text, 20.0f);

    expect(layout.lines.size() == 2, "combining clusters should not split");
    for (const auto& line : layout.lines) {
        expect(line.end - line.begin >= 3, "line contains base and combining mark");
    }
}

void testExplicitEmptyLines() {
    const std::string text = "a\n\nb";
    const LFTextLayout layout = buildLayout(text, 100.0f);

    expect(layout.lines.size() == 3, "explicit blank lines should be preserved");
    if (layout.lines.size() == 3) {
        expect(lineText(text, layout.lines[0]) == "a", "first explicit line");
        expect(layout.lines[1].begin == layout.lines[1].end, "blank line offsets");
        expect(lineText(text, layout.lines[2]) == "b", "last explicit line");
        expect(std::fabs(layout.height - 54.0f) < 0.001f,
               "empty lines retain inter-line advance");
    }
}

void testMaxLinesAndTruncation() {
    const std::string text = "alpha beta gamma delta";
    const LFTextLayout layout = buildLayout(text, 80.0f, 2);

    expect(layout.lines.size() == 2, "maxLines should limit line count");
    expect(layout.truncated, "maxLines should report truncation");
    expect(std::fabs(layout.height - 34.0f) < 0.001f,
           "maxLines excludes trailing external leading");
}

void testLongWordEmergencyWrap() {
    const std::string text = "abcdefghijklmnop";
    const LFTextLayout layout = buildLayout(text, 30.0f);

    expect(layout.lines.size() == 6, "long words wrap by grapheme");
    if (layout.lines.size() == 6) {
        expect(lineText(text, layout.lines[0]) == "abc", "first emergency line");
        expect(lineText(text, layout.lines[5]) == "p", "last emergency line");
    }
}

void testOpticalCenterUsesInkBounds() {
    LFTextLine line;
    line.advance = 100.0f;
    line.inkMinX = -10.0f;
    line.inkMaxX = 60.0f;

    const float opticalX =
        line.resolveOriginX(0.0f, 200.0f, LFTextLayoutAlign::Center);
    const float logicalX = (200.0f - line.advance) * 0.5f;

    expect(std::fabs(opticalX - 75.0f) < 0.001f, "optical center uses ink bounds");
    expect(std::fabs(opticalX - logicalX) > 0.001f, "ink bias is corrected");
}

void testInvalidUtf8IsTolerated() {
    const std::string text("a\xFFz", 3);
    const LFTextLayout layout = buildLayout(text, 100.0f);
    expect(!layout.lines.empty(), "invalid UTF-8 should still produce layout");
}

void testLayoutInvariantsAcrossWidths() {
    const std::vector<std::string> samples = {
        "alpha beta gamma delta epsilon",
        "中英文Mixed排版，标点必须稳定。",
        "👩‍💻 family 👨‍👩‍👧‍👦 flag 🇨🇳",
        "e\u0301lan\n\nsecond line",
        "a-really-long-unbreakable-token"
    };

    for (const std::string& text : samples) {
        for (float width = 0.0f; width <= 120.0f; width += 7.0f) {
            for (int maxLines : {0, 1, 3}) {
                const LFTextLayout layout = buildLayout(text, width, maxLines);
                size_t previousEnd = 0;
                for (const auto& line : layout.lines) {
                    expect(line.begin <= line.end, "line offsets are ordered");
                    expect(line.end <= text.size(), "line offsets stay in range");
                    expect(line.begin >= previousEnd, "lines make forward progress");
                    previousEnd = line.end;
                }
                if (maxLines > 0) {
                    expect(static_cast<int>(layout.lines.size()) <= maxLines,
                           "line count respects maxLines");
                }
            }
        }
    }
}

} // namespace

int main() {
    testLatinWordWrapping();
    testTrailingSpacesAreNotDrawn();
    testCjkPunctuationDoesNotStartLine();
    testEmojiSequenceStaysTogether();
    testCombiningMarkStaysWithBase();
    testExplicitEmptyLines();
    testMaxLinesAndTruncation();
    testLongWordEmergencyWrap();
    testOpticalCenterUsesInkBounds();
    testInvalidUtf8IsTolerated();
    testLayoutInvariantsAcrossWidths();

    if (failureCount != 0) {
        std::cerr << failureCount << " text layout test(s) failed\n";
        return 1;
    }
    std::cout << "All text layout tests passed\n";
    return 0;
}
