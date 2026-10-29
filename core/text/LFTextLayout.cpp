//
// Created by Chen Tong on 2026/10/29.
//

#include "text/LFTextLayout.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace {

constexpr uint32_t kReplacementCharacter = 0xFFFD;
constexpr float kFitEpsilon = 0.01f;

struct UTF8Codepoint {
    uint32_t value = kReplacementCharacter;
    size_t begin = 0;
    size_t end = 0;
};

struct GraphemeUnit {
    size_t begin = 0;
    size_t end = 0;
    uint32_t first = 0;
    uint32_t last = 0;
    int regionalIndicatorCount = 0;
    bool newline = false;
    bool space = false;
    bool cjk = false;
    bool openingPunctuation = false;
    bool closingPunctuation = false;
    bool breakableHyphen = false;
};

bool isUtf8Continuation(unsigned char value) {
    return (value & 0xC0) == 0x80;
}

size_t utf8SequenceLength(unsigned char first, size_t remaining) {
    size_t length = 1;
    if ((first & 0xE0) == 0xC0) {
        length = 2;
    } else if ((first & 0xF0) == 0xE0) {
        length = 3;
    } else if ((first & 0xF8) == 0xF0) {
        length = 4;
    }
    return std::min(length, remaining);
}

uint32_t decodeUtf8Codepoint(std::string_view text, size_t begin, size_t length) {
    const auto byteAt = [text](size_t index) {
        return static_cast<unsigned char>(text[index]);
    };

    if (length == 1) {
        return byteAt(begin) < 0x80 ? byteAt(begin) : kReplacementCharacter;
    }

    uint32_t value = 0;
    if (length == 2) {
        value = byteAt(begin) & 0x1F;
    } else if (length == 3) {
        value = byteAt(begin) & 0x0F;
    } else if (length == 4) {
        value = byteAt(begin) & 0x07;
    } else {
        return kReplacementCharacter;
    }

    for (size_t i = 1; i < length; ++i) {
        const unsigned char continuation = byteAt(begin + i);
        if (!isUtf8Continuation(continuation)) {
            return kReplacementCharacter;
        }
        value = (value << 6) | (continuation & 0x3F);
    }

    if ((length == 2 && value < 0x80) ||
        (length == 3 && value < 0x800) ||
        (length == 4 && value < 0x10000) ||
        value > 0x10FFFF ||
        (value >= 0xD800 && value <= 0xDFFF)) {
        return kReplacementCharacter;
    }
    return value;
}

std::vector<UTF8Codepoint> decodeUtf8(std::string_view text) {
    std::vector<UTF8Codepoint> codepoints;
    codepoints.reserve(text.size());

    size_t offset = 0;
    while (offset < text.size()) {
        const unsigned char first = static_cast<unsigned char>(text[offset]);
        size_t length = 1;
        if (first >= 0xC2 && first <= 0xF4) {
            length = utf8SequenceLength(first, text.size() - offset);
        }

        UTF8Codepoint decoded;
        decoded.begin = offset;
        decoded.end = offset + length;
        decoded.value = decodeUtf8Codepoint(text, offset, length);
        codepoints.push_back(decoded);
        offset = decoded.end;
    }
    return codepoints;
}

bool isNewline(uint32_t codepoint) {
    return codepoint == '\n' || codepoint == '\r' || codepoint == 0x0085 ||
           codepoint == 0x2028 || codepoint == 0x2029;
}

bool isSpace(uint32_t codepoint) {
    return codepoint == '\t' || codepoint == '\v' || codepoint == '\f' ||
           codepoint == 0x20 || codepoint == 0x1680 ||
           (codepoint >= 0x2000 && codepoint <= 0x2006) ||
           (codepoint >= 0x2008 && codepoint <= 0x200A) ||
           codepoint == 0x205F || codepoint == 0x3000;
}

bool isRegionalIndicator(uint32_t codepoint) {
    return codepoint >= 0x1F1E6 && codepoint <= 0x1F1FF;
}

bool isCombiningMark(uint32_t codepoint) {
    return (codepoint >= 0x0300 && codepoint <= 0x036F) ||
           (codepoint >= 0x0483 && codepoint <= 0x0489) ||
           (codepoint >= 0x0591 && codepoint <= 0x05BD) ||
           codepoint == 0x05BF ||
           (codepoint >= 0x05C1 && codepoint <= 0x05C2) ||
           (codepoint >= 0x0610 && codepoint <= 0x061A) ||
           (codepoint >= 0x064B && codepoint <= 0x065F) ||
           (codepoint >= 0x0670 && codepoint <= 0x0670) ||
           (codepoint >= 0x06D6 && codepoint <= 0x06ED) ||
           (codepoint >= 0x0730 && codepoint <= 0x074A) ||
           (codepoint >= 0x07A6 && codepoint <= 0x07B0) ||
           (codepoint >= 0x0816 && codepoint <= 0x082D) ||
           (codepoint >= 0x08D3 && codepoint <= 0x0903) ||
           (codepoint >= 0x093A && codepoint <= 0x094F) ||
           (codepoint >= 0x0951 && codepoint <= 0x0957) ||
           (codepoint >= 0x0962 && codepoint <= 0x0963) ||
           (codepoint >= 0x0981 && codepoint <= 0x0983) ||
           (codepoint >= 0x09BC && codepoint <= 0x09CD) ||
           (codepoint >= 0x09D7 && codepoint <= 0x09D7) ||
           (codepoint >= 0x09E2 && codepoint <= 0x09E3) ||
           (codepoint >= 0x0A01 && codepoint <= 0x0A03) ||
           (codepoint >= 0x0A3C && codepoint <= 0x0A51) ||
           (codepoint >= 0x0A70 && codepoint <= 0x0A71) ||
           (codepoint >= 0x0A75 && codepoint <= 0x0A75) ||
           (codepoint >= 0x0A81 && codepoint <= 0x0A83) ||
           (codepoint >= 0x0ABC && codepoint <= 0x0ACD) ||
           (codepoint >= 0x0AE2 && codepoint <= 0x0AE3) ||
           (codepoint >= 0x0B01 && codepoint <= 0x0B03) ||
           (codepoint >= 0x0B3C && codepoint <= 0x0B4D) ||
           (codepoint >= 0x0B56 && codepoint <= 0x0B57) ||
           (codepoint >= 0x0B62 && codepoint <= 0x0B63) ||
           (codepoint >= 0x0B82 && codepoint <= 0x0B82) ||
           (codepoint >= 0x0BBE && codepoint <= 0x0BCD) ||
           (codepoint >= 0x0C00 && codepoint <= 0x0C04) ||
           (codepoint >= 0x0C3E && codepoint <= 0x0C56) ||
           (codepoint >= 0x0C81 && codepoint <= 0x0C83) ||
           (codepoint >= 0x0CBC && codepoint <= 0x0CD6) ||
           (codepoint >= 0x0CE2 && codepoint <= 0x0CE3) ||
           (codepoint >= 0x0D00 && codepoint <= 0x0D03) ||
           (codepoint >= 0x0D3B && codepoint <= 0x0D4D) ||
           (codepoint >= 0x0D62 && codepoint <= 0x0D63) ||
           (codepoint >= 0x0DCA && codepoint <= 0x0DDF) ||
           (codepoint >= 0x0E31 && codepoint <= 0x0E31) ||
           (codepoint >= 0x0E34 && codepoint <= 0x0E3A) ||
           (codepoint >= 0x0E47 && codepoint <= 0x0E4E) ||
           (codepoint >= 0x0EB1 && codepoint <= 0x0EB1) ||
           (codepoint >= 0x0EB4 && codepoint <= 0x0EBC) ||
           (codepoint >= 0x0EC8 && codepoint <= 0x0ECD) ||
           (codepoint >= 0x0F18 && codepoint <= 0x0F19) ||
           (codepoint >= 0x0F35 && codepoint <= 0x0F35) ||
           (codepoint >= 0x0F37 && codepoint <= 0x0F37) ||
           (codepoint >= 0x0F39 && codepoint <= 0x0F39) ||
           (codepoint >= 0x0F71 && codepoint <= 0x0F84) ||
           (codepoint >= 0x0F86 && codepoint <= 0x0F87) ||
           (codepoint >= 0x0F8D && codepoint <= 0x0FBC) ||
           (codepoint >= 0x102D && codepoint <= 0x103E) ||
           (codepoint >= 0x1058 && codepoint <= 0x1059) ||
           (codepoint >= 0x105E && codepoint <= 0x1060) ||
           (codepoint >= 0x1071 && codepoint <= 0x1074) ||
           (codepoint >= 0x1082 && codepoint <= 0x108D) ||
           (codepoint >= 0x135D && codepoint <= 0x135F) ||
           (codepoint >= 0x1712 && codepoint <= 0x1714) ||
           (codepoint >= 0x1732 && codepoint <= 0x1734) ||
           (codepoint >= 0x1752 && codepoint <= 0x1753) ||
           (codepoint >= 0x1772 && codepoint <= 0x1773) ||
           (codepoint >= 0x17B4 && codepoint <= 0x17D3) ||
           (codepoint >= 0x180B && codepoint <= 0x180D) ||
           (codepoint >= 0x1920 && codepoint <= 0x193B) ||
           (codepoint >= 0x1A17 && codepoint <= 0x1A1B) ||
           (codepoint >= 0x1A56 && codepoint <= 0x1A7F) ||
           (codepoint >= 0x1AB0 && codepoint <= 0x1AFF) ||
           (codepoint >= 0x1B00 && codepoint <= 0x1B04) ||
           (codepoint >= 0x1B34 && codepoint <= 0x1B44) ||
           (codepoint >= 0x1B6B && codepoint <= 0x1B73) ||
           (codepoint >= 0x1B80 && codepoint <= 0x1B82) ||
           (codepoint >= 0x1BA2 && codepoint <= 0x1BAD) ||
           (codepoint >= 0x1BE6 && codepoint <= 0x1BF3) ||
           (codepoint >= 0x1C2C && codepoint <= 0x1C37) ||
           (codepoint >= 0x1CD0 && codepoint <= 0x1CE8) ||
           (codepoint >= 0x1DC0 && codepoint <= 0x1DFF) ||
           (codepoint >= 0x20D0 && codepoint <= 0x20FF) ||
           (codepoint >= 0xFE00 && codepoint <= 0xFE0F) ||
           (codepoint >= 0xFE20 && codepoint <= 0xFE2F) ||
           (codepoint >= 0x1F3FB && codepoint <= 0x1F3FF) ||
           (codepoint >= 0xE0020 && codepoint <= 0xE007F) ||
           (codepoint >= 0xE0100 && codepoint <= 0xE01EF);
}

bool isCjk(uint32_t codepoint) {
    return (codepoint >= 0x1100 && codepoint <= 0x11FF) ||
           (codepoint >= 0x2E80 && codepoint <= 0x2FFF) ||
           (codepoint >= 0x3040 && codepoint <= 0x30FF) ||
           (codepoint >= 0x3130 && codepoint <= 0x318F) ||
           (codepoint >= 0x31F0 && codepoint <= 0x31FF) ||
           (codepoint >= 0x3400 && codepoint <= 0x4DBF) ||
           (codepoint >= 0x4E00 && codepoint <= 0x9FFF) ||
           (codepoint >= 0xA960 && codepoint <= 0xA97F) ||
           (codepoint >= 0xAC00 && codepoint <= 0xD7AF) ||
           (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||
           (codepoint >= 0xFF00 && codepoint <= 0xFFEF) ||
           (codepoint >= 0x20000 && codepoint <= 0x3FFFF);
}

bool isOpeningPunctuation(uint32_t codepoint) {
    switch (codepoint) {
        case '(':
        case '[':
        case '{':
        case 0x2018:
        case 0x201C:
        case 0x3008:
        case 0x300A:
        case 0x300C:
        case 0x300E:
        case 0x3010:
        case 0x3014:
        case 0x3016:
        case 0x3018:
        case 0x301A:
        case 0xFF08:
        case 0xFF3B:
        case 0xFF5B:
            return true;
        default:
            return false;
    }
}

bool isClosingPunctuation(uint32_t codepoint) {
    switch (codepoint) {
        case ',':
        case '.':
        case ':':
        case ';':
        case '!':
        case '?':
        case ')':
        case ']':
        case '}':
        case 0x00B7:
        case 0x2019:
        case 0x201D:
        case 0x2026:
        case 0x3001:
        case 0x3002:
        case 0x3003:
        case 0x3009:
        case 0x300B:
        case 0x300D:
        case 0x300F:
        case 0x3011:
        case 0x3015:
        case 0x3017:
        case 0x3019:
        case 0x301B:
        case 0x301C:
        case 0x301D:
        case 0x301E:
        case 0x301F:
        case 0x30FB:
        case 0xFF01:
        case 0xFF09:
        case 0xFF0C:
        case 0xFF0D:
        case 0xFF0E:
        case 0xFF1A:
        case 0xFF1B:
        case 0xFF1F:
        case 0xFF3D:
        case 0xFF5D:
            return true;
        default:
            return false;
    }
}

bool isBreakableHyphen(uint32_t codepoint) {
    return codepoint == '-' || codepoint == 0x2010 ||
           codepoint == 0x2012 || codepoint == 0x2013;
}

bool isHangulLead(uint32_t codepoint) {
    return codepoint >= 0x1100 && codepoint <= 0x115F;
}

bool isHangulVowel(uint32_t codepoint) {
    return codepoint >= 0x1160 && codepoint <= 0x11A7;
}

bool isHangulTrail(uint32_t codepoint) {
    return codepoint >= 0x11A8 && codepoint <= 0x11FF;
}

bool isHangulLv(uint32_t codepoint) {
    return codepoint >= 0xAC00 && codepoint <= 0xD7A3 &&
           ((codepoint - 0xAC00) % 28) == 0;
}

bool isHangulLvt(uint32_t codepoint) {
    return codepoint >= 0xAC00 && codepoint <= 0xD7A3 &&
           ((codepoint - 0xAC00) % 28) != 0;
}

bool shouldJoinGrapheme(const GraphemeUnit& previous, uint32_t codepoint) {
    if (isCombiningMark(codepoint) || codepoint == 0x200D) {
        return true;
    }
    if (previous.last == 0x200D) {
        return true;
    }
    if (isRegionalIndicator(previous.last) && isRegionalIndicator(codepoint)) {
        return (previous.regionalIndicatorCount % 2) == 1;
    }
    if (isHangulLead(previous.last) &&
        (isHangulVowel(codepoint) || isHangulLv(codepoint))) {
        return true;
    }
    if ((isHangulVowel(previous.last) || isHangulLv(previous.last)) &&
        (isHangulTrail(codepoint) || isHangulLvt(codepoint))) {
        return true;
    }
    return false;
}

GraphemeUnit makeUnit(const UTF8Codepoint& codepoint) {
    GraphemeUnit unit;
    unit.begin = codepoint.begin;
    unit.end = codepoint.end;
    unit.first = codepoint.value;
    unit.last = codepoint.value;
    unit.newline = isNewline(codepoint.value);
    unit.space = isSpace(codepoint.value);
    unit.cjk = isCjk(codepoint.value);
    unit.openingPunctuation = isOpeningPunctuation(codepoint.value);
    unit.closingPunctuation = isClosingPunctuation(codepoint.value);
    unit.breakableHyphen = isBreakableHyphen(codepoint.value);
    unit.regionalIndicatorCount = isRegionalIndicator(codepoint.value) ? 1 : 0;
    return unit;
}

std::vector<GraphemeUnit> buildGraphemeUnits(std::string_view text) {
    const std::vector<UTF8Codepoint> codepoints = decodeUtf8(text);
    std::vector<GraphemeUnit> units;

    for (size_t index = 0; index < codepoints.size();) {
        const UTF8Codepoint& codepoint = codepoints[index];
        if (codepoint.value == '\r' && index + 1 < codepoints.size() &&
            codepoints[index + 1].value == '\n') {
            GraphemeUnit unit;
            unit.begin = codepoint.begin;
            unit.end = codepoints[index + 1].end;
            unit.first = '\n';
            unit.last = '\n';
            unit.newline = true;
            units.push_back(unit);
            index += 2;
            continue;
        }

        if (isNewline(codepoint.value)) {
            units.push_back(makeUnit(codepoint));
            ++index;
            continue;
        }

        GraphemeUnit unit = makeUnit(codepoint);
        while (index + 1 < codepoints.size() &&
               !isNewline(codepoints[index + 1].value) &&
               shouldJoinGrapheme(unit, codepoints[index + 1].value)) {
            ++index;
            unit.end = codepoints[index].end;
            unit.last = codepoints[index].value;
            unit.cjk = isCjk(codepoints[index].value);
            unit.openingPunctuation = isOpeningPunctuation(codepoints[index].value);
            unit.closingPunctuation = isClosingPunctuation(codepoints[index].value);
            unit.breakableHyphen = isBreakableHyphen(codepoints[index].value);
            if (isRegionalIndicator(codepoints[index].value)) {
                ++unit.regionalIndicatorCount;
            }
        }
        units.push_back(unit);
        ++index;
    }
    return units;
}

bool canBreakBetween(const GraphemeUnit& previous, const GraphemeUnit& next) {
    if (next.newline || previous.space) {
        return true;
    }
    if (next.closingPunctuation || previous.openingPunctuation) {
        return false;
    }
    if (previous.last == 0x200D || next.first == 0x200D) {
        return false;
    }
    if (previous.breakableHyphen || previous.first == 0x200B) {
        return true;
    }
    return previous.cjk || next.cjk;
}

size_t skipSpaces(const std::vector<GraphemeUnit>& units, size_t begin, size_t end) {
    while (begin < end && units[begin].space) {
        ++begin;
    }
    return begin;
}

size_t trimTrailingSpaces(const std::vector<GraphemeUnit>& units,
                          size_t begin,
                          size_t end) {
    while (end > begin && units[end - 1].space) {
        --end;
    }
    return end;
}

LFTextLineMetrics sanitizeMetrics(LFTextLineMetrics metrics) {
    if (!std::isfinite(metrics.advance) || metrics.advance < 0.0f) {
        metrics.advance = 0.0f;
    }
    if (!std::isfinite(metrics.inkMinX)) {
        metrics.inkMinX = 0.0f;
    }
    if (!std::isfinite(metrics.inkMaxX) || metrics.inkMaxX < metrics.inkMinX) {
        metrics.inkMaxX = metrics.inkMinX;
    }
    return metrics;
}

LFTextLine makeLine(std::string_view text,
                    const std::vector<GraphemeUnit>& units,
                    size_t beginUnit,
                    size_t endUnit,
                    const LFTextMeasureLine& measureLine) {
    LFTextLine line;
    if (beginUnit >= units.size() || endUnit <= beginUnit) {
        return line;
    }

    line.begin = units[beginUnit].begin;
    line.end = units[endUnit - 1].end;
    const LFTextLineMetrics metrics = sanitizeMetrics(
        measureLine(text.substr(line.begin, line.end - line.begin))
    );
    line.advance = metrics.advance;
    line.inkMinX = metrics.inkMinX;
    line.inkMaxX = metrics.inkMaxX;
    return line;
}

bool fits(float width, float wrapWidth) {
    return width <= wrapWidth + kFitEpsilon;
}

} // namespace

float LFTextLine::getInkWidth() const {
    return std::max(0.0f, inkMaxX - inkMinX);
}

float LFTextLine::resolveOriginX(float containerX,
                                 float containerWidth,
                                 LFTextLayoutAlign align) const {
    switch (align) {
        case LFTextLayoutAlign::Center: {
            const float inkWidth = getInkWidth();
            if (inkWidth > 0.0f) {
                return containerX + containerWidth * 0.5f -
                       (inkMinX + inkMaxX) * 0.5f;
            }
            return containerX + (containerWidth - advance) * 0.5f;
        }
        case LFTextLayoutAlign::Right:
            return containerX + containerWidth - advance;
        case LFTextLayoutAlign::Left:
        default:
            return containerX;
    }
}

LFTextLayout LFTextLayoutBuilder::build(std::string_view text,
                                        const LFTextLayoutOptions& options,
                                        const LFTextMeasureLine& measureLine) {
    LFTextLayout layout;
    if (text.empty() || !measureLine) {
        return layout;
    }

    const float lineAdvance =
        std::isfinite(options.lineAdvance) && options.lineAdvance > 0.0f
            ? options.lineAdvance
            : 1.0f;
    const float wrapWidth =
        std::isfinite(options.wrapWidth)
            ? std::max(0.0f, options.wrapWidth)
            : std::numeric_limits<float>::infinity();
    const int maxLines = std::max(0, options.maxLines);
    const std::vector<GraphemeUnit> units = buildGraphemeUnits(text);
    layout.lineAdvance = lineAdvance;

    const auto appendEmptyLine = [&layout, lineAdvance](size_t offset) {
        LFTextLine line;
        line.begin = offset;
        line.end = offset;
        layout.lines.push_back(line);
        layout.height += lineAdvance;
    };

    size_t unitIndex = 0;
    while (unitIndex < units.size()) {
        if (maxLines > 0 && static_cast<int>(layout.lines.size()) >= maxLines) {
            layout.truncated = true;
            break;
        }

        if (units[unitIndex].newline) {
            appendEmptyLine(units[unitIndex].begin);
            ++unitIndex;
            continue;
        }

        size_t paragraphEnd = unitIndex;
        while (paragraphEnd < units.size() && !units[paragraphEnd].newline) {
            ++paragraphEnd;
        }

        size_t cursor = skipSpaces(units, unitIndex, paragraphEnd);
        while (cursor < paragraphEnd) {
            if (maxLines > 0 && static_cast<int>(layout.lines.size()) >= maxLines) {
                layout.truncated = true;
                break;
            }

            cursor = skipSpaces(units, cursor, paragraphEnd);
            if (cursor >= paragraphEnd) {
                break;
            }

            bool foundFit = false;
            size_t chosenRawEnd = cursor;
            size_t chosenDrawEnd = cursor;
            LFTextLine chosenLine;

            for (size_t candidateEnd = cursor + 1;
                 candidateEnd <= paragraphEnd;
                 ++candidateEnd) {
                const bool paragraphFinished = candidateEnd == paragraphEnd;
                if (!paragraphFinished &&
                    !canBreakBetween(units[candidateEnd - 1], units[candidateEnd])) {
                    continue;
                }

                const size_t drawEnd = trimTrailingSpaces(units, cursor, candidateEnd);
                if (drawEnd <= cursor) {
                    continue;
                }

                LFTextLine candidate = makeLine(
                    text, units, cursor, drawEnd, measureLine
                );
                if (fits(candidate.advance, wrapWidth)) {
                    foundFit = true;
                    chosenRawEnd = candidateEnd;
                    chosenDrawEnd = drawEnd;
                    chosenLine = candidate;
                    continue;
                }
                break;
            }

            if (!foundFit) {
                for (size_t emergencyEnd = cursor + 1;
                     emergencyEnd <= paragraphEnd;
                     ++emergencyEnd) {
                    LFTextLine candidate = makeLine(
                        text, units, cursor, emergencyEnd, measureLine
                    );
                    if (!fits(candidate.advance, wrapWidth) &&
                        chosenDrawEnd > cursor) {
                        break;
                    }

                    chosenRawEnd = emergencyEnd;
                    chosenDrawEnd = emergencyEnd;
                    chosenLine = candidate;
                    if (!fits(candidate.advance, wrapWidth)) {
                        break;
                    }
                }

                while (chosenRawEnd < paragraphEnd &&
                       units[chosenRawEnd].closingPunctuation) {
                    ++chosenRawEnd;
                    chosenDrawEnd = chosenRawEnd;
                    chosenLine = makeLine(
                        text, units, cursor, chosenDrawEnd, measureLine
                    );
                }
            }

            if (chosenRawEnd <= cursor) {
                chosenRawEnd = std::min(cursor + 1, paragraphEnd);
                chosenDrawEnd = chosenRawEnd;
                chosenLine = makeLine(
                    text, units, cursor, chosenDrawEnd, measureLine
                );
            }

            layout.lines.push_back(chosenLine);
            layout.width = std::max(layout.width, chosenLine.advance);
            layout.height += lineAdvance;
            cursor = chosenRawEnd;
        }

        if (maxLines > 0 && static_cast<int>(layout.lines.size()) >= maxLines &&
            (cursor < paragraphEnd || paragraphEnd < units.size())) {
            layout.truncated = true;
            break;
        }

        unitIndex = paragraphEnd;
        if (unitIndex < units.size() && units[unitIndex].newline) {
            ++unitIndex;
        }
    }
    return layout;
}
