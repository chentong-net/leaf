//
// Created by Chen Tong on 2026/1/17.
//

#include "view/base/LFText.h"

#include "LFEngine.h"
#include "text/LFTextLayout.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

NVGcontext* LFText::s_measureContext = nullptr;

namespace {

constexpr size_t kLayoutCacheCapacity = 128;

struct LayoutCacheKey {
    std::string text;
    int fontId = -1;
    float fontSize = 0.0f;
    float lineHeight = 0.0f;
    float wrapWidth = 0.0f;
    float pixelRatio = 1.0f;
    int maxLines = 0;

    bool operator==(const LayoutCacheKey& other) const {
        return text == other.text &&
               fontId == other.fontId &&
               fontSize == other.fontSize &&
               lineHeight == other.lineHeight &&
               wrapWidth == other.wrapWidth &&
               pixelRatio == other.pixelRatio &&
               maxLines == other.maxLines;
    }
};

void hashCombine(size_t& seed, size_t value) {
    seed ^= value + 0x9E3779B97F4A7C15ULL + (seed << 6) + (seed >> 2);
}

struct LayoutCacheKeyHash {
    size_t operator()(const LayoutCacheKey& key) const {
        size_t seed = std::hash<std::string>{}(key.text);
        hashCombine(seed, std::hash<int>{}(key.fontId));
        hashCombine(seed, std::hash<float>{}(key.fontSize));
        hashCombine(seed, std::hash<float>{}(key.lineHeight));
        hashCombine(seed, std::hash<float>{}(key.wrapWidth));
        hashCombine(seed, std::hash<float>{}(key.pixelRatio));
        hashCombine(seed, std::hash<int>{}(key.maxLines));
        return seed;
    }
};

struct LayoutCacheEntry {
    std::shared_ptr<const LFTextLayout> layout;
    std::list<std::string>::iterator recency;
};

class TextLayoutCache {
public:
    std::shared_ptr<const LFTextLayout> get(const LayoutCacheKey& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_entries.find(key);
        if (it == m_entries.end()) {
            return nullptr;
        }
        m_recency.splice(m_recency.begin(), m_recency, it->second.recency);
        return it->second.layout;
    }

    void put(const LayoutCacheKey& key, std::shared_ptr<const LFTextLayout> layout) {
        if (!layout) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        auto existing = m_entries.find(key);
        if (existing != m_entries.end()) {
            existing->second.layout = std::move(layout);
            m_recency.splice(m_recency.begin(), m_recency, existing->second.recency);
            return;
        }

        m_recency.push_front(key.text);
        m_entries.emplace(key, LayoutCacheEntry{std::move(layout), m_recency.begin()});
        while (m_entries.size() > kLayoutCacheCapacity && !m_recency.empty()) {
            const auto oldest = std::prev(m_recency.end());
            for (auto entry = m_entries.begin(); entry != m_entries.end(); ++entry) {
                if (entry->second.recency == oldest) {
                    m_entries.erase(entry);
                    break;
                }
            }
            m_recency.erase(oldest);
        }
    }

private:
    std::mutex m_mutex;
    std::list<std::string> m_recency;
    std::unordered_map<LayoutCacheKey, LayoutCacheEntry, LayoutCacheKeyHash> m_entries;
};

TextLayoutCache& getTextLayoutCache() {
    static TextLayoutCache cache;
    return cache;
}

float sanitizeLineHeight(float lineHeight) {
    return std::isfinite(lineHeight) && lineHeight > 0.0f ? lineHeight : 1.0f;
}

float sanitizeEdge(float value) {
    if (!std::isfinite(value) || YGFloatIsUndefined(value)) {
        return 0.0f;
    }
    return std::max(0.0f, value);
}

float snapToPixel(float value, float pixelRatio) {
    if (!std::isfinite(value) || !std::isfinite(pixelRatio) || pixelRatio <= 0.0f) {
        return value;
    }
    return std::round(value * pixelRatio) / pixelRatio;
}

float canonicalizeWidth(float width, float pixelRatio) {
    if (!std::isfinite(width)) {
        return width;
    }
    return snapToPixel(std::max(0.0f, width), pixelRatio);
}

float reserveOneDevicePixel(float value, float pixelRatio) {
    if (!std::isfinite(value) || !std::isfinite(pixelRatio) || pixelRatio <= 0.0f) {
        return value;
    }
    return std::ceil((std::max(0.0f, value) + 1.0f / pixelRatio) * pixelRatio) /
           pixelRatio;
}

LFTextLayoutAlign toLayoutAlign(LFTextHAlign align) {
    switch (align) {
        case LFTextHAlign::Center:
            return LFTextLayoutAlign::Center;
        case LFTextHAlign::Right:
            return LFTextLayoutAlign::Right;
        case LFTextHAlign::Left:
        default:
            return LFTextLayoutAlign::Left;
    }
}

const char* resolveFontFamily(NVGcontext* vg, const std::string& fontFamily) {
    if (!vg) {
        return nullptr;
    }
    if (nvgFindFont(vg, fontFamily.c_str()) >= 0) {
        return fontFamily.c_str();
    }
    return nvgFindFont(vg, "sans") >= 0 ? "sans" : nullptr;
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

std::shared_ptr<const LFTextLayout> getTextLayout(
        NVGcontext* vg,
        const std::string& text,
        const std::string& fontFamily,
        float fontSize,
        float lineHeight,
        float wrapWidth,
        int maxLines) {
    if (!vg || text.empty()) {
        return nullptr;
    }

    const char* resolvedFontFamily = resolveFontFamily(vg, fontFamily);
    if (!resolvedFontFamily) {
        return nullptr;
    }

    const int fontId = nvgFindFont(vg, resolvedFontFamily);
    const float pixelRatio = std::max(0.01f, LFEngine::getInstance().getPixelRatio());
    const float effectiveWrapWidth = canonicalizeWidth(wrapWidth, pixelRatio);

    LayoutCacheKey key;
    key.text = text;
    key.fontId = fontId;
    key.fontSize = fontSize;
    key.lineHeight = sanitizeLineHeight(lineHeight);
    key.wrapWidth = effectiveWrapWidth;
    key.pixelRatio = pixelRatio;
    key.maxLines = std::max(0, maxLines);

    auto& cache = getTextLayoutCache();
    if (auto cached = cache.get(key)) {
        return cached;
    }

    nvgSave(vg);
    nvgFontFace(vg, resolvedFontFamily);
    nvgFontSize(vg, fontSize);
    nvgTextLineHeight(vg, key.lineHeight);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    float naturalLineHeight = 0.0f;
    nvgTextMetrics(vg, nullptr, nullptr, &naturalLineHeight);
    if (!std::isfinite(naturalLineHeight) || naturalLineHeight <= 0.0f) {
        naturalLineHeight = std::max(1.0f, fontSize);
    }

    LFTextLayoutOptions options;
    options.wrapWidth = effectiveWrapWidth;
    options.maxLines = key.maxLines;
    options.lineAdvance = std::max(1.0f, naturalLineHeight * key.lineHeight);

    auto layout = std::make_shared<LFTextLayout>(
        LFTextLayoutBuilder::build(
            text,
            options,
            [vg](std::string_view line) {
                return measureLine(vg, line);
            }
        )
    );
    nvgRestore(vg);

    cache.put(key, layout);
    return layout;
}

} // namespace

LFText::LFText() {
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
    const float safeLineHeight = sanitizeLineHeight(lineHeight);
    if (m_lineHeight == safeLineHeight) return;
    m_lineHeight = safeLineHeight;
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

void LFText::setMaxLines(int maxLines) {
    const int safeMaxLines = std::max(0, maxLines);
    if (m_hasMaxLinesConfigured && m_maxLines == safeMaxLines) return;
    m_maxLines = safeMaxLines;
    m_hasMaxLinesConfigured = true;
    YGNodeMarkDirty(getYGNode());
    markDirty();
}

YGSize LFText::measure(YGNodeRef node, float width, YGMeasureMode widthMode,
                       float height, YGMeasureMode heightMode) {
    (void)height;
    (void)heightMode;

    auto* baseNode = static_cast<LFNode*>(YGNodeGetContext(node));
    auto* textNode = static_cast<LFText*>(baseNode);
    NVGcontext* vg = s_measureContext;
    if (!vg || !textNode || textNode->m_text.empty()) {
        return {0.0f, 0.0f};
    }

    float wrapWidth = std::numeric_limits<float>::infinity();
    if (widthMode == YGMeasureModeExactly || widthMode == YGMeasureModeAtMost) {
        wrapWidth = std::max(0.0f, width);
    }

    const auto layout = getTextLayout(
        vg,
        textNode->m_text,
        textNode->m_fontFamily,
        textNode->m_fontSize,
        textNode->m_lineHeight,
        wrapWidth,
        textNode->m_maxLines
    );
    if (!layout) {
        return {0.0f, 0.0f};
    }

    const float pixelRatio = std::max(0.01f, LFEngine::getInstance().getPixelRatio());
    YGSize result{layout->width, layout->height};
    if (widthMode == YGMeasureModeExactly) {
        result.width = width;
    } else {
        const float naturalWidth = reserveOneDevicePixel(layout->width, pixelRatio);
        result.width = widthMode == YGMeasureModeAtMost
                           ? std::min(width, naturalWidth)
                           : naturalWidth;
    }
    result.height = reserveOneDevicePixel(layout->height, pixelRatio);
    return result;
}

void LFText::onDrawContent(NVGcontext* vg) {
    if (m_text.empty() || !vg) return;

    const float paddingLeft = sanitizeEdge(
        YGNodeLayoutGetPadding(getYGNode(), YGEdgeLeft)
    );
    const float paddingTop = sanitizeEdge(
        YGNodeLayoutGetPadding(getYGNode(), YGEdgeTop)
    );
    const float paddingRight = sanitizeEdge(
        YGNodeLayoutGetPadding(getYGNode(), YGEdgeRight)
    );
    const float paddingBottom = sanitizeEdge(
        YGNodeLayoutGetPadding(getYGNode(), YGEdgeBottom)
    );
    const float borderLeft = sanitizeEdge(
        YGNodeLayoutGetBorder(getYGNode(), YGEdgeLeft)
    );
    const float borderTop = sanitizeEdge(
        YGNodeLayoutGetBorder(getYGNode(), YGEdgeTop)
    );
    const float borderRight = sanitizeEdge(
        YGNodeLayoutGetBorder(getYGNode(), YGEdgeRight)
    );
    const float borderBottom = sanitizeEdge(
        YGNodeLayoutGetBorder(getYGNode(), YGEdgeBottom)
    );

    const float contentX = paddingLeft + borderLeft;
    const float contentY = paddingTop + borderTop;
    const float contentWidth = getLayoutWidth() - contentX - paddingRight - borderRight;
    const float contentHeight = getLayoutHeight() - contentY - paddingBottom - borderBottom;
    if (contentWidth < 0.0f || contentHeight < 0.0f) {
        return;
    }

    const auto layout = getTextLayout(
        vg,
        m_text,
        m_fontFamily,
        m_fontSize,
        m_lineHeight,
        contentWidth,
        m_maxLines
    );
    if (!layout || layout->empty()) {
        return;
    }

    float drawY = contentY;
    switch (m_textVAlign) {
        case LFTextVAlign::Center:
            drawY = contentY + (contentHeight - layout->height) * 0.5f;
            break;
        case LFTextVAlign::Bottom:
            drawY = contentY + contentHeight - layout->height;
            break;
        case LFTextVAlign::Top:
        default:
            drawY = contentY;
            break;
    }

    const char* resolvedFontFamily = resolveFontFamily(vg, m_fontFamily);
    if (!resolvedFontFamily) {
        return;
    }

    nvgSave(vg);
    nvgFontFace(vg, resolvedFontFamily);
    nvgFontSize(vg, m_fontSize);
    nvgTextLineHeight(vg, m_lineHeight);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgFillColor(vg, colorToNVG(m_textColor));

    const float pixelRatio = std::max(0.01f, LFEngine::getInstance().getPixelRatio());
    const LFTextLayoutAlign layoutAlign = toLayoutAlign(m_textHAlign);
    for (size_t index = 0; index < layout->lines.size(); ++index) {
        const LFTextLine& line = layout->lines[index];
        if (line.begin >= line.end || line.end > m_text.size()) {
            continue;
        }

        const float originX = snapToPixel(
            line.resolveOriginX(contentX, contentWidth, layoutAlign),
            pixelRatio
        );
        const float lineY = drawY + static_cast<float>(index) * layout->lineAdvance;
        nvgText(
            vg,
            originX,
            lineY,
            m_text.data() + line.begin,
            m_text.data() + line.end
        );
    }

    nvgRestore(vg);
}
