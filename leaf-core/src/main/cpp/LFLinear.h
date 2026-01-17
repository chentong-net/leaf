//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFLINEAR_H
#define LEAF_LFLINEAR_H

#include "LFRenderNode.h"

// 布局方向：水平或垂直
enum class LFLinearDirection { Horizontal, Vertical };

// 对齐方式：主轴 (Justify) 和 交叉轴 (Align)
enum class LFJustify { Start, Center, End, SpaceBetween, SpaceAround };
enum class LFAlign { Auto, Start, Center, End, Stretch };

class LFLinear : public LFRenderNode {
public:
    LFLinear();
    virtual ~LFLinear() = default;

    // 核心属性设置
    void setDirection(LFLinearDirection dir);
    void setJustifyContent(LFJustify justify);
    void setAlignItems(LFAlign align);

    void setGap(float gap);

    void setBackgroundColor(uint32_t color);
    void setBorderRadius(float radius);

    // 快捷静态工厂
    static std::shared_ptr<LFLinear> createHorizontal();
    static std::shared_ptr<LFLinear> createVertical();

protected:
    void onDraw(NVGcontext* vg) override;

private:
    LFLinearDirection m_direction = LFLinearDirection::Vertical;
    uint32_t m_backgroundColor = 0x00000000; // 默认透明
    float m_borderRadius = 0.0f;
};

#endif
