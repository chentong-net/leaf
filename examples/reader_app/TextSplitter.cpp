//
// Created by Chen Tong on 2026/1/31.
//

#include "TextSplitter.h"

#include "text/LFTextLayout.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>
#include <utility>

namespace {

struct SplitMetrics {
    float lineAdvance = 0.0f;
    float naturalLineHeight = 0.0f;
    float firstBaseline = 0.0f;
    int linesPerPage = 1;
};

LFFontHandle resolveFont(LFRenderer* renderer, const std::string& fontFamily) {
    if (!renderer) {
        return 0;
    }
    LFFontHandle font = renderer->findFont(fontFamily);
    if (font == 0) {
        font = renderer->findFont("sans");
    }
    return font;
}

bool prepareSplitMetrics(LFRenderer* renderer,
                         const SplitConfig& config,
                         SplitMetrics& metrics) {
    if (!renderer || !std::isfinite(config.width) || config.width <= 0.0f ||
        !std::isfinite(config.height) || config.height <= 0.0f ||
        !std::isfinite(config.fontSize) || config.fontSize <= 0.0f) {
        return false;
    }

    const LFFontHandle font = resolveFont(renderer, config.fontFamily);
    if (font == 0) {
        return false;
    }

    const float lineHeightScale =
        std::isfinite(config.lineHeight) && config.lineHeight > 0.0f
            ? config.lineHeight
            : 1.0f;
    const LFFontMetrics fontMetrics = renderer->measureFontMetrics(
        font,
        config.fontSize,
        lineHeightScale
    );
    float ascender = fontMetrics.ascender;
    float naturalLineHeight = fontMetrics.lineHeight;
    if (!std::isfinite(naturalLineHeight) || naturalLineHeight <= 0.0f) {
        naturalLineHeight = config.fontSize;
    }
    if (!std::isfinite(ascender) || ascender <= 0.0f) {
        ascender = naturalLineHeight;
    }

    metrics.lineAdvance = std::max(1.0f, naturalLineHeight * lineHeightScale);
    metrics.naturalLineHeight = naturalLineHeight;
    metrics.firstBaseline = ascender;
    metrics.linesPerPage = std::max(
        1,
        static_cast<int>(std::floor(config.height / metrics.lineAdvance))
    );
    return true;
}

LFTextLineMetrics measureLine(LFRenderer* renderer,
                              LFFontHandle font,
                              float fontSize,
                              float lineHeight,
                              std::string_view text) {
    LFTextLineMetrics metrics;
    if (!renderer || font == 0 || text.empty()) {
        return metrics;
    }

    const LFTextBounds bounds = renderer->measureTextInkBounds(
        font, fontSize, lineHeight, text);
    metrics.advance = bounds.advance;
    metrics.inkMinX = bounds.minX;
    metrics.inkMaxX = bounds.maxX;
    metrics.inkMinY = bounds.minY;
    metrics.inkMaxY = bounds.maxY;
    return metrics;
}

LFTextLayout buildLayout(LFRenderer* renderer,
                         LFFontHandle font,
                         std::string_view text,
                         const SplitConfig& config,
                         const SplitMetrics& metrics,
                         int maxLines) {
    if (!renderer || font == 0) {
        return {};
    }

    const float pixelRatio = std::max(0.01f, LFEngine::getInstance().getPixelRatio());
    const float width = std::round(config.width * pixelRatio) / pixelRatio;
    const float lineHeightScale =
        std::isfinite(config.lineHeight) && config.lineHeight > 0.0f
            ? config.lineHeight
            : 1.0f;

    LFTextLayoutOptions options;
    options.wrapWidth = width;
    options.maxLines = maxLines;
    options.lineAdvance = metrics.lineAdvance;
    options.naturalLineHeight = metrics.naturalLineHeight;
    options.firstBaseline = metrics.firstBaseline;
    return LFTextLayoutBuilder::build(
        text,
        options,
        [renderer, font, &config, lineHeightScale](std::string_view line) {
            return measureLine(
                renderer,
                font,
                config.fontSize,
                lineHeightScale,
                line
            );
        }
    );
}

std::vector<SplitPage> buildPages(std::string_view text,
                                  size_t baseOffset,
                                  const LFTextLayout& layout,
                                  const SplitMetrics& metrics,
                                  int maxPages) {
    std::vector<SplitPage> pages;
    if (text.empty() || layout.lines.empty() || metrics.linesPerPage <= 0) {
        return pages;
    }

    const int pageLimit = maxPages > 0 ? maxPages : std::numeric_limits<int>::max();
    for (size_t firstLine = 0;
         firstLine < layout.lines.size() && static_cast<int>(pages.size()) < pageLimit;
         firstLine += static_cast<size_t>(metrics.linesPerPage)) {
        const size_t afterLastLine = std::min(
            layout.lines.size(),
            firstLine + static_cast<size_t>(metrics.linesPerPage)
        );
        const size_t pageStart = layout.lines[firstLine].begin;
        size_t pageEnd = afterLastLine < layout.lines.size()
                             ? layout.lines[afterLastLine].begin
                             : text.size();
        if (pageEnd <= pageStart) {
            pageEnd = std::min(text.size(), layout.lines[afterLastLine - 1].end);
        }
        if (pageEnd <= pageStart) {
            pageEnd = std::min(text.size(), pageStart + 1);
        }

        SplitPage page;
        page.text.assign(text.data() + pageStart, pageEnd - pageStart);
        page.startOffset = baseOffset + pageStart;
        page.endOffset = baseOffset + pageEnd;
        pages.push_back(std::move(page));

        if (afterLastLine >= layout.lines.size()) {
            break;
        }
    }
    return pages;
}

std::vector<SplitPage> splitRange(std::string_view text,
                                  size_t baseOffset,
                                  const SplitConfig& config,
                                  int maxPages) {
    LFRenderer* renderer = LFEngine::getInstance().getRenderer();
    if (!renderer || text.empty()) {
        return {};
    }

    SplitMetrics metrics;
    if (!prepareSplitMetrics(renderer, config, metrics)) {
        return {};
    }

    const int pageLimit = maxPages > 0 ? maxPages : std::numeric_limits<int>::max();
    const int64_t requestedLines =
        static_cast<int64_t>(metrics.linesPerPage) * pageLimit + 1;
    const int maxLines = static_cast<int>(
        std::min<int64_t>(requestedLines, std::numeric_limits<int>::max())
    );
    const LFFontHandle font = resolveFont(renderer, config.fontFamily);
    const LFTextLayout layout = buildLayout(
        renderer,
        font,
        text,
        config,
        metrics,
        maxLines
    );
    const std::vector<SplitPage> pages =
        buildPages(text, baseOffset, layout, metrics, maxPages);
    return pages;
}

} // namespace

std::vector<SplitPage> TextSplitter::split(const std::string& fullText,
                                           const SplitConfig& config) {
    if (fullText.empty()) {
        return {};
    }
    return splitRange(fullText, 0, config, 0);
}

std::vector<SplitPage> TextSplitter::splitStep(SplitIterator& iter,
                                               const SplitConfig& config,
                                               int batchSize) {
    if (iter.isFinished || !iter.basePos || !iter.currentPos || !iter.endPos ||
        iter.currentPos >= iter.endPos || batchSize <= 0) {
        iter.isFinished = true;
        return {};
    }

    while (iter.currentPos < iter.endPos &&
           (static_cast<unsigned char>(*iter.currentPos) & 0xC0) == 0x80) {
        ++iter.currentPos;
    }
    if (iter.currentPos >= iter.endPos) {
        iter.isFinished = true;
        return {};
    }

    const size_t rawOffset = static_cast<size_t>(iter.currentPos - iter.basePos);
    const std::string_view remaining(
        iter.currentPos,
        static_cast<size_t>(iter.endPos - iter.currentPos)
    );
    std::vector<SplitPage> pages = splitRange(remaining, rawOffset, config, batchSize);
    if (pages.empty()) {
        iter.currentPos = iter.endPos;
        iter.isFinished = true;
        return {};
    }

    const size_t pageEnd = pages.back().endOffset - rawOffset;
    const char* next = iter.currentPos + std::min(pageEnd, remaining.size());
    if (next <= iter.currentPos) {
        next = iter.currentPos + 1;
    }
    iter.currentPos = std::min(next, iter.endPos);
    iter.isFinished = iter.currentPos >= iter.endPos;
    return pages;
}
