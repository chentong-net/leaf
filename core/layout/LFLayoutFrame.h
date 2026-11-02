#ifndef LEAF_LFLAYOUTFRAME_H
#define LEAF_LFLAYOUTFRAME_H

#include <limits>

struct LFEdgeInsets {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

/**
 * 一次布局计算后，节点在父布局坐标系中的结果。
 */
struct LFLayoutFrame {
    float x = 0.0f;
    float y = 0.0f;
    float width = std::numeric_limits<float>::quiet_NaN();
    float height = std::numeric_limits<float>::quiet_NaN();
    LFEdgeInsets padding;
    LFEdgeInsets border;
};

#endif // LEAF_LFLAYOUTFRAME_H
