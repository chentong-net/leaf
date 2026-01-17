//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFFLEX_H
#define LEAF_LFFLEX_H

#include "LFRenderNode.h"

/**
 * LFFlex: 工业级通用弹性布局容器
 * 1. 它是所有复杂组件的基石（类似 Web 的 div 或 Flutter 的 Container）。
 * 2. 默认开启 Yoga 的弹性布局能力。
 * 3. 作为子节点“绝对定位”的参考原点（通过 LFRenderNode 的坐标平移实现）。
 */
class LFFlex : public LFRenderNode {
public:
    LFFlex();
    virtual ~LFFlex() = default;

    void setBackgroundColor(uint32_t color);
    void setBorderRadius(float radius);

protected:
    void onDraw(NVGcontext* vg) override;

private:
    uint32_t m_backgroundColor = 0x00000000; // 默认透明
    float m_borderRadius = 0.0f;
};

#endif
