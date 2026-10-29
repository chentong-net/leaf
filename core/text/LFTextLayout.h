//
// Created by Chen Tong on 2026/10/29.
//

#ifndef LEAF_LFTEXTLAYOUT_H
#define LEAF_LFTEXTLAYOUT_H

#include <cstddef>
#include <functional>
#include <limits>
#include <string_view>
#include <vector>

enum class LFTextLayoutAlign {
    Left,
    Center,
    Right
};

/**
* 单行文本的字形度量
*/
struct LFTextLineMetrics {
    float advance = 0.0f;
    float inkMinX = 0.0f;
    float inkMaxX = 0.0f;
};

using LFTextMeasureLine = std::function<LFTextLineMetrics(std::string_view)>;

/**
* 文本布局参数
*/
struct LFTextLayoutOptions {
    float wrapWidth = std::numeric_limits<float>::infinity();
    int maxLines = 0;
    float lineAdvance = 1.0f;
};

/**
* 完成断行后的单行结果
*/
struct LFTextLine {
    size_t begin = 0;
    size_t end = 0;
    float advance = 0.0f;
    float inkMinX = 0.0f;
    float inkMaxX = 0.0f;

    float getInkWidth() const;

    /**
    * Center使用字形着墨边界居中
    */
    float resolveOriginX(float containerX,
                         float containerWidth,
                         LFTextLayoutAlign align) const;
};

/**
* Yoga测量和NanoVG绘制共同使用的文本布局
*/
struct LFTextLayout {
    std::vector<LFTextLine> lines;
    float width = 0.0f;
    float height = 0.0f;
    float lineAdvance = 0.0f;
    bool truncated = false;

    bool empty() const { return lines.empty(); }
};

/**
* 文本布局构建器
*/
class LFTextLayoutBuilder {
public:
    static LFTextLayout build(std::string_view text,
                              const LFTextLayoutOptions& options,
                              const LFTextMeasureLine& measureLine);
};

#endif // LEAF_LFTEXTLAYOUT_H
