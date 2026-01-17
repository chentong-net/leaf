//
// Created by Chen Tong on 2026/1/18.
//

#include "LFFlex.h"

LFFlex::LFFlex() {
    // 默认设置为垂直布局 (Column)
    // 确保任何放入 LFFlex 的子节点默认按列排列
    YGNodeStyleSetFlexDirection(m_ygNode, YGFlexDirectionColumn);
}

void LFFlex::setBackgroundColor(uint32_t color) {
    if (m_backgroundColor == color) return;
    m_backgroundColor = color;
    markDirty(); // 背景色改变仅需重绘，无需重新计算 Yoga 布局
}

void LFFlex::setBorderRadius(float radius) {
    if (m_borderRadius == radius) return;
    m_borderRadius = radius;
    markDirty();
}

void LFFlex::onDraw(NVGcontext* vg) {
    // 获取由 Yoga 计算出的当前节点尺寸
    float w = getLayoutWidth();
    float h = getLayoutHeight();

    // 健壮性检查：无效尺寸不进行绘制
    if (w <= 0 || h <= 0) return;

    // 绘制背景色与圆角
    // 检查 Alpha 通道，如果完全透明则跳过绘制指令，优化性能
    if ((m_backgroundColor >> 24) & 0xFF) {
        nvgBeginPath(vg);
        if (m_borderRadius > 0) {
            nvgRoundedRect(vg, 0, 0, w, h, m_borderRadius);
        } else {
            nvgRect(vg, 0, 0, w, h);
        }
        nvgFillColor(vg, colorToNVG(m_backgroundColor)); // 使用基类的颜色转换工具
        nvgFill(vg);
    }
}
