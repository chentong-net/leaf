//
// Created by Chen Tong on 2026/1/17.
//

#include "view/base/LFText.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <vector>

// Initialize static member.
NVGcontext* LFText::s_measureContext = nullptr;

namespace {

struct LFTextRowLayout {
    const char* start = nullptr;
    const char* end = nullptr;
    float minX = 0.0f;
    float maxX = 0.0f;
    float logicalWidth = 0.0f;
};

struct LFTextLayoutResult {
    std::vector<LFTextRowLayout> rows;
    float lineAdvance = 0.0f;
    float textHeight = 0.0f;
    float maxVisualWidth = 0.0f;
};

float sanitizeEdgeValue(float value) {
    if (std::isnan(value) || YGFloatIsUndefined(value)) {
        return 0.0f;
    }
    return std::max(0.0f, value);
}

float rowVisualWidth(const LFTextRowLayout& row) {
    return std::max(0.0f, row.maxX - row.minX);
}

void applyTextState(NVGcontext* vg,
                    float fontSize,
                    const std::string& fontFamily,
                    float lineHeight) {
    nvgFontSize(vg, fontSize);
    nvgFontFace(vg, fontFamily.c_str());
    nvgTextLineHeight(vg, lineHeight);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
}

bool measureRowByGlyphs(NVGcontext* vg,
                        const char* start,
                        const char* end,
                        LFTextRowLayout& outRow) {
    if (!vg || !start || !end || start >= end) return false;

    const int glyphCapacity = std::max(1, static_cast<int>(end - start) + 1);
    std::vector<NVGglyphPosition> glyphs(static_cast<size_t>(glyphCapacity));
    const int glyphCount = nvgTextGlyphPositions(vg, 0.0f, 0.0f, start, end, glyphs.data(), glyphCapacity);

    outRow.start = start;
    outRow.end = end;
    outRow.logicalWidth = nvgTextBounds(vg, 0.0f, 0.0f, start, end, nullptr);
    if (glyphCount <= 0) {
        float bounds[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        nvgTextBounds(vg, 0.0f, 0.0f, start, end, bounds);
        outRow.minX = bounds[0];
        outRow.maxX = bounds[2];
        return true;
    }

    float minX = glyphs[0].minx;
    float maxX = glyphs[0].maxx;
    for (int i = 1; i < glyphCount; ++i) {
        minX = std::min(minX, glyphs[i].minx);
        maxX = std::max(maxX, glyphs[i].maxx);
    }
    outRow.minX = minX;
    outRow.maxX = maxX;
    return true;
}

bool isAsciiBlank(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

void trimLeadingAsciiBlanks(const char*& cursor, const char* end) {
    while (cursor < end && isAsciiBlank(*cursor)) {
        ++cursor;
    }
}

void appendHardWrappedRow(NVGcontext* vg,
                          const char* start,
                          const char* end,
                          float wrapWidth,
                          std::vector<LFTextRowLayout>& outRows,
                          float& maxVisualWidth) {
    if (!vg || !start || !end || start >= end) return;
    if (wrapWidth <= 0.0f || !std::isfinite(wrapWidth)) {
        LFTextRowLayout row;
        if (measureRowByGlyphs(vg, start, end, row)) {
            maxVisualWidth = std::max(maxVisualWidth, rowVisualWidth(row));
            outRows.push_back(row);
        }
        return;
    }

    const float kWrapEpsilon = 0.01f;
    const char* cursor = start;
    while (cursor < end) {
        const int glyphCapacity = std::max(1, static_cast<int>(end - cursor) + 1);
        std::vector<NVGglyphPosition> glyphs(static_cast<size_t>(glyphCapacity));
        const int glyphCount = nvgTextGlyphPositions(vg, 0.0f, 0.0f, cursor, end, glyphs.data(), glyphCapacity);
        if (glyphCount <= 0) {
            LFTextRowLayout row;
            if (measureRowByGlyphs(vg, cursor, end, row)) {
                maxVisualWidth = std::max(maxVisualWidth, rowVisualWidth(row));
                outRows.push_back(row);
            }
            break;
        }

        int bestCount = 1;
        float minX = glyphs[0].minx;
        float maxX = glyphs[0].maxx;
        float bestMinX = minX;
        float bestMaxX = maxX;

        for (int i = 1; i < glyphCount; ++i) {
            minX = std::min(minX, glyphs[i].minx);
            maxX = std::max(maxX, glyphs[i].maxx);
            const float visualWidth = maxX - minX;
            if (visualWidth <= wrapWidth + kWrapEpsilon) {
                bestCount = i + 1;
                bestMinX = minX;
                bestMaxX = maxX;
            } else {
                break;
            }
        }

        const char* segmentEnd = (bestCount < glyphCount) ? glyphs[bestCount].str : end;
        if (segmentEnd <= cursor) {
            segmentEnd = (glyphCount > 1) ? glyphs[1].str : end;
            if (segmentEnd <= cursor) {
                break;
            }
        }

        LFTextRowLayout row;
        row.start = cursor;
        row.end = segmentEnd;
        row.minX = bestMinX;
        row.maxX = bestMaxX;
        row.logicalWidth = nvgTextBounds(vg, 0.0f, 0.0f, row.start, row.end, nullptr);
        maxVisualWidth = std::max(maxVisualWidth, rowVisualWidth(row));
        outRows.push_back(row);

        cursor = segmentEnd;
        trimLeadingAsciiBlanks(cursor, end);
    }
}

LFTextLayoutResult buildGreedyLayout(NVGcontext* vg,
                                     const std::string& text,
                                     float fontSize,
                                     const std::string& fontFamily,
                                     float lineHeight,
                                     float wrapWidth) {
    LFTextLayoutResult layout;
    if (!vg || text.empty()) return layout;

    applyTextState(vg, fontSize, fontFamily, lineHeight);

    float baseLineHeight = 0.0f;
    nvgTextMetrics(vg, nullptr, nullptr, &baseLineHeight);
    layout.lineAdvance = baseLineHeight * lineHeight;

    const char* begin = text.c_str();
    const char* end = begin + text.size();
    // Keep a small wrap slack to avoid precision-triggered premature line breaks.
    const float wrapSafetyPx = std::max(8.0f, fontSize * 0.10f);
    const float safeWrapWidth = wrapWidth > 0.0f
        ? (wrapWidth + wrapSafetyPx)
        : (std::numeric_limits<float>::max() / 4.0f);

    NVGtextRow rows[16];
    while (begin < end) {
        const int nrows = nvgTextBreakLines(vg, begin, end, safeWrapWidth, rows, 16);
        if (nrows <= 0) break;

        for (int i = 0; i < nrows; ++i) {
            const float visualWidth = std::max(0.0f, rows[i].maxx - rows[i].minx);
            if (safeWrapWidth > 0.0f && visualWidth > safeWrapWidth + 0.01f) {
                appendHardWrappedRow(
                    vg,
                    rows[i].start,
                    rows[i].end,
                    safeWrapWidth,
                    layout.rows,
                    layout.maxVisualWidth
                );
                continue;
            }

            LFTextRowLayout row;
            row.start = rows[i].start;
            row.end = rows[i].end;
            row.minX = rows[i].minx;
            row.maxX = rows[i].maxx;
            row.logicalWidth = rows[i].width;
            layout.maxVisualWidth = std::max(layout.maxVisualWidth, rowVisualWidth(row));
            layout.rows.push_back(row);
        }

        const char* next = rows[nrows - 1].next;
        if (next <= begin) {
            break;
        }
        begin = next;
    }

    if (!layout.rows.empty()) {
        layout.textHeight = layout.lineAdvance * static_cast<float>(layout.rows.size());
    }
    return layout;
}

float balancedLayoutScore(const LFTextLayoutResult& layout,
                          float containerWidth,
                          size_t baselineRows) {
    if (layout.rows.empty() || containerWidth <= 0.0f) {
        return std::numeric_limits<float>::max();
    }

    const float invWidth = 1.0f / std::max(containerWidth, 1.0f);
    constexpr float kTargetFill = 0.72f;

    float sum = 0.0f;
    for (const auto& row : layout.rows) {
        sum += rowVisualWidth(row);
    }
    const float mean = sum / static_cast<float>(layout.rows.size());

    float variance = 0.0f;
    float fullnessPenalty = 0.0f;
    float overflowPenalty = 0.0f;
    float maxRatio = 0.0f;
    for (const auto& row : layout.rows) {
        const float w = rowVisualWidth(row);
        const float diff = w - mean;
        variance += diff * diff;

        const float ratio = w * invWidth;
        maxRatio = std::max(maxRatio, ratio);

        const float fillDiff = ratio - kTargetFill;
        fullnessPenalty += fillDiff * fillDiff;

        if (ratio > 0.88f) {
            const float over = ratio - 0.88f;
            fullnessPenalty += over * over * 3.0f;
        }
        if (ratio > 1.0f) {
            const float over = ratio - 1.0f;
            overflowPenalty += over * over * 12.0f;
        }
    }

    float tailPenalty = 0.0f;
    if (layout.rows.size() > 1) {
        const float lastRatio = rowVisualWidth(layout.rows.back()) * invWidth;
        if (lastRatio < 0.45f) {
            const float under = 0.45f - lastRatio;
            tailPenalty = under * under * 2.0f;
        }
    }

    const float rowsDelta = static_cast<float>(
        std::abs(static_cast<int>(layout.rows.size()) - static_cast<int>(baselineRows))
    );
    const float rowsPenalty = rowsDelta * 0.08f;

    const float normalizedVariance = variance * invWidth * invWidth;
    const float maxRatioPenalty = std::max(0.0f, maxRatio - 0.90f);
    return normalizedVariance
           + fullnessPenalty
           + overflowPenalty
           + tailPenalty
           + rowsPenalty
           + maxRatioPenalty * maxRatioPenalty * 4.0f;
}

LFTextLayoutResult buildBalancedLayout(NVGcontext* vg,
                                       const std::string& text,
                                       float fontSize,
                                       const std::string& fontFamily,
                                       float lineHeight,
                                       float wrapWidth,
                                       LFTextHAlign hAlign) {
    LFTextLayoutResult baseline = buildGreedyLayout(vg, text, fontSize, fontFamily, lineHeight, wrapWidth);
    if (hAlign != LFTextHAlign::Center || wrapWidth <= 0.0f || baseline.rows.size() <= 1) {
        return baseline;
    }

    LFTextLayoutResult best = baseline;
    float bestScore = balancedLayoutScore(best, wrapWidth, baseline.rows.size());

    static const float kCandidateFactors[] = {
        0.95f, 0.90f, 0.85f, 0.80f, 0.75f, 0.70f, 0.65f, 0.60f, 0.55f
    };
    const float minWrapWidth = std::max(fontSize * 2.0f, wrapWidth * 0.50f);

    for (float factor : kCandidateFactors) {
        const float candidateWrap = std::max(minWrapWidth, wrapWidth * factor);
        if (candidateWrap >= wrapWidth) continue;

        LFTextLayoutResult candidate = buildGreedyLayout(
            vg, text, fontSize, fontFamily, lineHeight, candidateWrap
        );
        if (candidate.rows.empty()) continue;
        if (candidate.rows.size() > baseline.rows.size() + 2) continue;

        const float score = balancedLayoutScore(candidate, wrapWidth, baseline.rows.size());
        if (score < bestScore) {
            bestScore = score;
            best = std::move(candidate);
        }
    }

    return best;
}

LFTextLayoutResult buildTextLayout(NVGcontext* vg,
                                   const std::string& text,
                                   float fontSize,
                                   const std::string& fontFamily,
                                   float lineHeight,
                                   float wrapWidth,
                                   LFTextHAlign hAlign) {
    return buildBalancedLayout(vg, text, fontSize, fontFamily, lineHeight, wrapWidth, hAlign);
}

} // namespace

LFText::LFText() {
    // Only leaf nodes (Text/Image) need custom Yoga measure callbacks.
    YGNodeSetMeasureFunc(getYGNode(), LFText::measure);
}

void LFText::setText(const std::string& text) {
    if (m_text == text) return;
    m_text = text;
    YGNodeMarkDirty(getYGNode());
    markDirty();
}

void LFText::setFontSize(float size) {
    const float safeSize = std::max(1.0f, size);
    if (m_fontSize == safeSize) return;
    m_fontSize = safeSize;
    YGNodeMarkDirty(getYGNode());
    markDirty();
}

void LFText::setTextColor(uint32_t color) {
    if (m_textColor == color) return;
    m_textColor = color;
    markDirty();
}

void LFText::setLineHeight(float lineHeight) {
    if (m_lineHeight == lineHeight) return;
    m_lineHeight = lineHeight;
    YGNodeMarkDirty(getYGNode());
    markDirty();
}

void LFText::setFontFamily(const std::string& family) {
    if (m_fontFamily == family) return;
    m_fontFamily = family;
    YGNodeMarkDirty(getYGNode());
    markDirty();
}

void LFText::setTextHAlign(LFTextHAlign align) {
    if (m_textHAlign == align) return;
    m_textHAlign = align;
    markDirty();
}

void LFText::setTextVAlign(LFTextVAlign align) {
    if (m_textVAlign == align) return;
    m_textVAlign = align;
    markDirty();
}

YGSize LFText::measure(YGNodeRef node, float width, YGMeasureMode widthMode,
                       float /*height*/, YGMeasureMode /*heightMode*/) {
    auto* baseNode = static_cast<LFNode*>(YGNodeGetContext(node));
    auto* textNode = static_cast<LFText*>(baseNode);
    NVGcontext* vg = s_measureContext;

    if (!vg || !textNode || textNode->m_text.empty()) {
        return {0.0f, 0.0f};
    }

    const float pl = sanitizeEdgeValue(YGNodeLayoutGetPadding(node, YGEdgeLeft));
    const float pr = sanitizeEdgeValue(YGNodeLayoutGetPadding(node, YGEdgeRight));
    const float pt = sanitizeEdgeValue(YGNodeLayoutGetPadding(node, YGEdgeTop));
    const float pb = sanitizeEdgeValue(YGNodeLayoutGetPadding(node, YGEdgeBottom));
    const float bl = sanitizeEdgeValue(YGNodeLayoutGetBorder(node, YGEdgeLeft));
    const float br = sanitizeEdgeValue(YGNodeLayoutGetBorder(node, YGEdgeRight));
    const float bt = sanitizeEdgeValue(YGNodeLayoutGetBorder(node, YGEdgeTop));
    const float bb = sanitizeEdgeValue(YGNodeLayoutGetBorder(node, YGEdgeBottom));

    const float extraW = pl + pr + bl + br;
    const float extraH = pt + pb + bt + bb;

    float wrapWidth = 0.0f;
    if (widthMode == YGMeasureModeExactly || widthMode == YGMeasureModeAtMost) {
        wrapWidth = std::max(0.0f, width);
    }

    nvgSave(vg);
    const LFTextLayoutResult layout = buildTextLayout(
        vg,
        textNode->m_text,
        textNode->m_fontSize,
        textNode->m_fontFamily,
        textNode->m_lineHeight,
        wrapWidth,
        textNode->m_textHAlign
    );
    nvgRestore(vg);

    YGSize result{0.0f, 0.0f};
    // Small measurement slack prevents edge-case wrapping caused by float truncation.
    const float measureSafetyPx = std::max(1.0f, textNode->m_fontSize * 0.10f);
    if (widthMode == YGMeasureModeExactly) {
        result.width = width;
    } else if (widthMode == YGMeasureModeAtMost) {
        result.width = std::min(width, std::ceil(layout.maxVisualWidth + measureSafetyPx));
    } else {
        result.width = std::ceil(layout.maxVisualWidth + measureSafetyPx);
    }
    result.height = std::ceil(layout.textHeight);

    result.width += extraW;
    result.height += extraH;
    return result;
}

void LFText::onDrawContent(NVGcontext* vg) {
    if (m_text.empty()) return;

    const float paddingL = sanitizeEdgeValue(YGNodeLayoutGetPadding(getYGNode(), YGEdgeLeft));
    const float paddingT = sanitizeEdgeValue(YGNodeLayoutGetPadding(getYGNode(), YGEdgeTop));
    const float paddingR = sanitizeEdgeValue(YGNodeLayoutGetPadding(getYGNode(), YGEdgeRight));
    const float paddingB = sanitizeEdgeValue(YGNodeLayoutGetPadding(getYGNode(), YGEdgeBottom));
    const float borderL = sanitizeEdgeValue(YGNodeLayoutGetBorder(getYGNode(), YGEdgeLeft));
    const float borderT = sanitizeEdgeValue(YGNodeLayoutGetBorder(getYGNode(), YGEdgeTop));
    const float borderR = sanitizeEdgeValue(YGNodeLayoutGetBorder(getYGNode(), YGEdgeRight));
    const float borderB = sanitizeEdgeValue(YGNodeLayoutGetBorder(getYGNode(), YGEdgeBottom));

    const float contentX = paddingL + borderL;
    const float contentY = paddingT + borderT;

    const float contentW = getLayoutWidth() - contentX - paddingR - borderR;
    const float contentH = getLayoutHeight() - contentY - paddingB - borderB;
    if (contentW <= 0.0f || contentH <= 0.0f) return;

    nvgFillColor(vg, LFNode::colorToNVG(m_textColor));
    applyTextState(vg, m_fontSize, m_fontFamily, m_lineHeight);

    const LFTextLayoutResult layout = buildTextLayout(
        vg,
        m_text,
        m_fontSize,
        m_fontFamily,
        m_lineHeight,
        contentW,
        m_textHAlign
    );
    if (layout.rows.empty()) return;

    float drawY = contentY;
    switch (m_textVAlign) {
        case LFTextVAlign::Top:
            drawY = contentY;
            break;
        case LFTextVAlign::Center:
            drawY = contentY + (contentH - layout.textHeight) * 0.5f;
            break;
        case LFTextVAlign::Bottom:
            drawY = contentY + (contentH - layout.textHeight);
            break;
    }

    for (size_t i = 0; i < layout.rows.size(); ++i) {
        const LFTextRowLayout& row = layout.rows[i];
        const float visualWidth = rowVisualWidth(row);
        const float logicalWidth = std::max(row.logicalWidth, visualWidth);
        const float visualCenter = (row.minX + row.maxX) * 0.5f;

        float drawX = contentX - row.minX;
        switch (m_textHAlign) {
            case LFTextHAlign::Left:
                drawX = contentX - row.minX;
                break;
            case LFTextHAlign::Center: {
                // Use pure visual-center alignment to avoid hidden bias.
                drawX = contentX + contentW * 0.5f - visualCenter;
                // Snap to the nearest half pixel (not floor), avoiding systematic left shift.
                drawX = std::floor(drawX * 2.0f) * 0.5f;
                break;
            }
            case LFTextHAlign::Right:
                drawX = contentX + contentW - logicalWidth;
                break;
        }

        const float rowY = drawY + static_cast<float>(i) * layout.lineAdvance;
        nvgText(vg, drawX, rowY, row.start, row.end);
    }
}
