#ifndef LEAF_LFRENDERTYPES_H
#define LEAF_LFRENDERTYPES_H

#include <cstddef>
#include <cstdint>
#include <string_view>

using LFFontHandle = uint64_t;
using LFTextureHandle = uint64_t;

struct LFDrawRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct LFFontMetrics {
    float ascender = 0.0f;
    float descender = 0.0f;
    float lineHeight = 0.0f;
};

struct LFTextBounds {
    float advance = 0.0f;
    float minX = 0.0f;
    float maxX = 0.0f;
    float minY = 0.0f;
    float maxY = 0.0f;
};

enum class LFTextOrigin {
    Top,
    Baseline
};

enum class LFStrokeCap {
    Butt,
    Round,
    Square
};

struct LFTextDraw {
    LFFontHandle font = 0;
    std::string_view text;
    float x = 0.0f;
    float y = 0.0f;
    float fontSize = 16.0f;
    float lineHeight = 1.0f;
    uint32_t color = 0xFF000000;
    LFTextOrigin origin = LFTextOrigin::Baseline;
    bool italic = false;
};

struct LFImageDraw {
    LFTextureHandle texture = 0;
    LFDrawRect imageRect;
    LFDrawRect clipRect;
    float radius = 0.0f;
};

struct LFShadowDraw {
    LFDrawRect rect;
    float radius = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float blur = 0.0f;
    float spread = 0.0f;
    uint32_t color = 0x00000000;
};

#endif // LEAF_LFRENDERTYPES_H
