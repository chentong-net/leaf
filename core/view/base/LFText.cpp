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

LFRenderer* LFText::s_measureContext = nullptr;

namespace {

constexpr size_t kLayoutCacheCapacity = 128;

struct LayoutCacheKey {
    std::string text;
    LFFontHandle font = 0;
    float fontSize = 0.0f;
    float lineHeight = 0.0f;
    float wrapWidth = 0.0f;
    float pixelRatio = 1.0f;
    int maxLines = 0;

    bool operator==(const LayoutCacheKey& other) const {
        return text == other.text &&
               font == other.font &&
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
        hashCombine(seed, std::hash<LFFontHandle>{}(key.font));
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

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_entries.clear();
        m_recency.clear();
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

struct TextContentBox {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

TextContentBox getTextContentBox(YGNodeRef node, float layoutWidth, float layoutHeight) {
    const float paddingLeft = sanitizeEdge(
        YGNodeLayoutGetPadding(node, YGEdgeLeft)
    );
    const float paddingTop = sanitizeEdge(
        YGNodeLayoutGetPadding(node, YGEdgeTop)
    );
    const float paddingRight = sanitizeEdge(
        YGNodeLayoutGetPadding(node, YGEdgeRight)
    );
    const float paddingBottom = sanitizeEdge(
        YGNodeLayoutGetPadding(node, YGEdgeBottom)
    );
    const float borderLeft = sanitizeEdge(
        YGNodeLayoutGetBorder(node, YGEdgeLeft)
    );
    const float borderTop = sanitizeEdge(
        YGNodeLayoutGetBorder(node, YGEdgeTop)
    );
    const float borderRight = sanitizeEdge(
        YGNodeLayoutGetBorder(node, YGEdgeRight)
    );
    const float borderBottom = sanitizeEdge(
        YGNodeLayoutGetBorder(node, YGEdgeBottom)
    );

    TextContentBox content;
    content.x = paddingLeft + borderLeft;
    content.y = paddingTop + borderTop;
    content.width = std::max(
        0.0f,
        layoutWidth - content.x - paddingRight - borderRight
    );
    content.height = std::max(
        0.0f,
        layoutHeight - content.y - paddingBottom - borderBottom
    );
    return content;
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

float ceilToDevicePixel(float value, float pixelRatio) {
    if (!std::isfinite(value) || !std::isfinite(pixelRatio) || pixelRatio <= 0.0f) {
        return value;
    }
    return std::ceil(std::max(0.0f, value) * pixelRatio) / pixelRatio;
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

std::shared_ptr<const LFTextLayout> getTextLayout(
        LFRenderer* renderer,
        const std::string& text,
        const std::string& fontFamily,
        float fontSize,
        float lineHeight,
        float wrapWidth,
        int maxLines) {
    if (!renderer || text.empty()) {
        return nullptr;
    }

    const LFFontHandle font = resolveFont(renderer, fontFamily);
    if (font == 0) {
        return nullptr;
    }

    const float pixelRatio = std::max(0.01f, LFEngine::getInstance().getPixelRatio());
    const float effectiveWrapWidth = canonicalizeWidth(wrapWidth, pixelRatio);

    LayoutCacheKey key;
    key.text = text;
    key.font = font;
    key.fontSize = fontSize;
    key.lineHeight = sanitizeLineHeight(lineHeight);
    key.wrapWidth = effectiveWrapWidth;
    key.pixelRatio = pixelRatio;
    key.maxLines = std::max(0, maxLines);

    auto& cache = getTextLayoutCache();
    if (auto cached = cache.get(key)) {
        return cached;
    }

    const LFFontMetrics fontMetrics = renderer->measureFontMetrics(
        font, fontSize, key.lineHeight);
    float ascender = fontMetrics.ascender;
    float descender = fontMetrics.descender;
    float naturalLineHeight = fontMetrics.lineHeight;
    if (!std::isfinite(naturalLineHeight) || naturalLineHeight <= 0.0f) {
        naturalLineHeight = std::max(1.0f, fontSize);
    }
    if (!std::isfinite(ascender) || ascender <= 0.0f) {
        ascender = naturalLineHeight;
    }
    if (!std::isfinite(descender) || descender > 0.0f) {
        descender = ascender - naturalLineHeight;
    }

    LFTextLayoutOptions options;
    options.wrapWidth = effectiveWrapWidth;
    options.maxLines = key.maxLines;
    options.lineAdvance = std::max(1.0f, naturalLineHeight * key.lineHeight);
    options.naturalLineHeight = naturalLineHeight;
    options.firstBaseline = ascender;

    auto layout = std::make_shared<LFTextLayout>(
        LFTextLayoutBuilder::build(
            text,
            options,
            [renderer, font, fontSize, lineHeight = key.lineHeight](
                    std::string_view line) {
                return measureLine(renderer, font, fontSize, lineHeight, line);
            }
        )
    );

    cache.put(key, layout);
    return layout;
}

} // namespace

LFText::LFText() {
    YGNodeSetMeasureFunc(getYGNode(), LFText::measure);
    YGNodeSetBaselineFunc(getYGNode(), LFText::baseline);
}

void LFText::setMeasureContext(LFRenderer* renderer) {
    if (s_measureContext == renderer) {
        return;
    }

    getTextLayoutCache().clear();
    s_measureContext = renderer;
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

void LFText::setFontWeight(LFFontWeight weight) {
    if (m_fontWeight == weight) return;
    m_fontWeight = weight;
    YGNodeMarkDirty(getYGNode());
    markDirty();
}

std::string LFText::fontFamilyForStyle() const {
    if (m_fontFamily == kLFDefaultFontFamily) {
        return fontFamilyForWeight(m_fontWeight);
    }
    return m_fontFamily;
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

void LFText::setItalic(bool italic) {
    if (m_italic == italic) return;
    m_italic = italic;
    markDirty();
}

void LFText::setUnderline(bool underline) {
    if (m_underline == underline) return;
    m_underline = underline;
    markDirty();
}

void LFText::setHoverUnderline(bool enabled) {
    if (m_hoverUnderline == enabled) return;
    m_hoverUnderline = enabled;
    markDirty();
}

YGSize LFText::measure(YGNodeRef node, float width, YGMeasureMode widthMode,
                       float height, YGMeasureMode heightMode) {
    (void)height;
    (void)heightMode;

    auto* baseNode = static_cast<LFNode*>(YGNodeGetContext(node));
    auto* textNode = static_cast<LFText*>(baseNode);
    LFRenderer* renderer = s_measureContext;
    if (!renderer || !textNode || textNode->m_text.empty()) {
        return {0.0f, 0.0f};
    }

    float wrapWidth = std::numeric_limits<float>::infinity();
    if (widthMode == YGMeasureModeExactly || widthMode == YGMeasureModeAtMost) {
        wrapWidth = std::max(0.0f, width);
    }

    const auto layout = getTextLayout(
        renderer,
        textNode->m_text,
        textNode->fontFamilyForStyle(),
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
    result.height = ceilToDevicePixel(layout->height, pixelRatio);
    return result;
}

float LFText::baseline(YGNodeRef node, float width, float height) {
    auto* baseNode = static_cast<LFNode*>(YGNodeGetContext(node));
    auto* textNode = static_cast<LFText*>(baseNode);
    if (!textNode || textNode->m_text.empty()) {
        return std::max(0.0f, height);
    }

    const TextContentBox content = getTextContentBox(node, width, height);

    const auto layout = getTextLayout(
        s_measureContext,
        textNode->m_text,
        textNode->fontFamilyForStyle(),
        textNode->m_fontSize,
        textNode->m_lineHeight,
        content.width,
        textNode->m_maxLines
    );
    if (!layout || layout->empty()) {
        return std::max(0.0f, height);
    }

    float drawY = content.y;
    switch (textNode->m_textVAlign) {
        case LFTextVAlign::Center:
            drawY += (content.height - layout->height) * 0.5f;
            break;
        case LFTextVAlign::Bottom:
            drawY += content.height - layout->height;
            break;
        case LFTextVAlign::Top:
        default:
            break;
    }
    return drawY + layout->firstBaseline;
}

void LFText::onDrawContent(LFPaint& paint) {
    LFRenderer* renderer = LFEngine::getInstance().getRenderer();
    if (m_text.empty() || !renderer) return;

    const TextContentBox content = getTextContentBox(
        getYGNode(),
        getLayoutWidth(),
        getLayoutHeight()
    );
    if (content.width <= 0.0f || content.height <= 0.0f) {
        return;
    }

    const auto layout = getTextLayout(
        renderer,
        m_text,
        fontFamilyForStyle(),
        m_fontSize,
        m_lineHeight,
        content.width,
        m_maxLines
    );
    if (!layout || layout->empty()) {
        return;
    }

    float drawY = content.y;
    switch (m_textVAlign) {
        case LFTextVAlign::Center:
            drawY = content.y + (content.height - layout->height) * 0.5f;
            break;
        case LFTextVAlign::Bottom:
            drawY = content.y + content.height - layout->height;
            break;
        case LFTextVAlign::Top:
        default:
            drawY = content.y;
            break;
    }

    const LFFontHandle font = resolveFont(renderer, fontFamilyForStyle());
    if (font == 0) {
        return;
    }

    const float firstBaseline = drawY + layout->firstBaseline;
    const float pixelRatio = std::max(0.01f, LFEngine::getInstance().getPixelRatio());
    const LFTextLayoutAlign layoutAlign = toLayoutAlign(m_textHAlign);
    for (size_t index = 0; index < layout->lines.size(); ++index) {
        const LFTextLine& line = layout->lines[index];
        if (line.begin >= line.end || line.end > m_text.size()) {
            continue;
        }

        const float originX = snapToPixel(
            line.resolveOriginX(content.x, content.width, layoutAlign),
            pixelRatio
        );
        const float lineY =
            firstBaseline + static_cast<float>(index) * layout->lineAdvance;
        LFTextDraw draw;
        draw.font = font;
        draw.text = std::string_view(m_text).substr(line.begin, line.end - line.begin);
        draw.x = originX;
        draw.y = lineY;
        draw.fontSize = m_fontSize;
        draw.lineHeight = m_lineHeight;
        draw.color = m_textColor;
        draw.origin = LFTextOrigin::Baseline;
        draw.italic = m_italic;
        paint.drawText(draw);

        if (m_underline || (m_hoverUnderline && isHovered())) {
            const float underlineHeight = std::max(1.0f, 1.0f / pixelRatio);
            const float underlineY = lineY + std::max(1.0f, m_fontSize * 0.08f);
            paint.drawRect(
                {originX, underlineY, line.advance, underlineHeight},
                m_textColor
            );
        }
    }
}
