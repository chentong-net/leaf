//
// Created by Chen Tong on 2026/1/17.
//

#ifndef LEAF_LFNODE_H
#define LEAF_LFNODE_H

#include "LFRenderNode.h"

/**
 * LFNode 是 Leaf 引擎中最通用的具体实现类
 */
class LFNode : public LFRenderNode {
public:
    LFNode() = default;
    virtual ~LFNode() = default;

    // 视觉属性设置
    void setBackgroundColor(uint32_t color);
    void setCornerRadius(float radius);

    // 边框设置
    void setBorder(float width, uint32_t color);

protected:
    void onDraw(NVGcontext* vg) override;

private:
    uint32_t m_backgroundColor = 0x00000000;
    float m_cornerRadius = 0.0f;

    float m_borderWidth = 0.0f;
    uint32_t m_borderColor = 0x00000000;

    // 内部辅助：颜色转换逻辑
    static NVGcolor colorToNVG(uint32_t argb);
};

#endif // LEAF_LFNODE_H
