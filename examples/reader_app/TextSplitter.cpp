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
    int linesPerPage = 1;
};

const char* resolveFontFamily(NVGcontext* vg, const std::string& fontFamily) {
    if (!vg) {
        return nullptr;
    }
    if (nvgFindFont(vg, fontFamily.c_str()) >= 0) {
        return fontFamily.c_str();
    }
    return nvgFindFont(vg, "sans") >= 0 ? "sans" : nullptr;
}

bool prepareSplitMetrics(NVGcontext* vg,
                         const SplitConfig& config,
                         SplitMetrics& metrics) {
    if (!vg || !std::isfinite(config.width) || config.width <= 0.0f ||
        !std::isfinite(config.height) || config.height <= 0.0f ||
        !std::isfinite(config.fontSize) || config.fontSize <= 0.0f) {
        return false;
    }

    const char* fontFamily = resolveFontFamily(vg, config.fontFamily);
    if (!fontFamily) {
        return false;
    }

    const float lineHeightScale =
        std::isfinite(config.lineHeight) && config.lineHeight > 0.0f
            ? config.lineHeight
            : 1.0f;
    float naturalLineHeight = 0.0f;
    nvgFontFace(vg, fontFamily);
    nvgFontSize(vg, config.fontSize);
    nvgTextLineHeight(vg, lineHeightScale);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgTextMetrics(vg, nullptr, nullptr, &naturalLineHeight);
    if (!std::isfinite(naturalLineHeight) || naturalLineHeight <= 0.0f) {
        naturalLineHeight = config.fontSize;
    }

    metrics.lineAdvance = std::max(1.0f, naturalLineHeight * lineHeightScale);
    metrics.linesPerPage = std::max(
        1,
        static_cast<int>(std::floor(config.height / metrics.lineAdvance))
    );
    return true;
}

LFTextLineMetrics measureLine(NVGcontext* vg, std::string_view text) {
    LFTextLineMetrics metrics;
    if (!vg || text.empty()) {
        return metrics;
    }

    float bounds[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    metrics.advance = nvgTextBounds(
        vg,
        0.0f,
        0.0f,
        text.data(),
        text.data() + text.size(),
        bounds
    );
    metrics.inkMinX = bounds[0];
    metrics.inkMaxX = bounds[2];
    return metrics;
}

LFTextLayout buildLayout(NVGcontext* vg,
                         std::string_view text,
                         const SplitConfig& config,
                         const SplitMetrics& metrics,
                         int maxLines) {
    const char* fontFamily = resolveFontFamily(vg, config.fontFamily);
    if (!fontFamily) {
        return {};
    }

    const float pixelRatio = std::max(0.01f, LFEngine::getInstance().getPixelRatio());
    const float width = std::round(config.width * pixelRatio) / pixelRatio;
    const float lineHeightScale =
        std::isfinite(config.lineHeight) && config.lineHeight > 0.0f
            ? config.lineHeight
            : 1.0f;

    nvgFontFace(vg, fontFamily);
    nvgFontSize(vg, config.fontSize);
    nvgTextLineHeight(vg, lineHeightScale);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    LFTextLayoutOptions options;
    options.wrapWidth = width;
    options.maxLines = maxLines;
    options.lineAdvance = metrics.lineAdvance;
    return LFTextLayoutBuilder::build(
        text,
        options,
        [vg](std::string_view line) {
            return measureLine(vg, line);
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
    NVGcontext* vg = LFEngine::getInstance().getNVGContext();
    if (!vg || text.empty()) {
        return {};
    }

    nvgSave(vg);
    SplitMetrics metrics;
    if (!prepareSplitMetrics(vg, config, metrics)) {
        nvgRestore(vg);
        return {};
    }

    const int pageLimit = maxPages > 0 ? maxPages : std::numeric_limits<int>::max();
    const int64_t requestedLines =
        static_cast<int64_t>(metrics.linesPerPage) * pageLimit + 1;
    const int maxLines = static_cast<int>(
        std::min<int64_t>(requestedLines, std::numeric_limits<int>::max())
    );
    const LFTextLayout layout = buildLayout(vg, text, config, metrics, maxLines);
    const std::vector<SplitPage> pages =
        buildPages(text, baseOffset, layout, metrics, maxPages);
    nvgRestore(vg);
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
