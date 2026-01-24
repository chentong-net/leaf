//
// Created by Chen Tong on 2026/1/24.
//

#include "LFEvaluator.h"

// 辅助宏：限制数值范围 (Clamping)
#ifndef CLAMP
#define CLAMP(v, min, max) ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))
#endif

// ==========================================
// Color Evaluator (ARGB Interpolation)
// ==========================================
uint32_t LFColorEvaluator::evaluate(float fraction, const uint32_t& startValue, const uint32_t& endValue) {
    // 提取通道 (ARGB)
    // 格式假设为 0xAARRGGBB
    int startA = (startValue >> 24) & 0xff;
    int startR = (startValue >> 16) & 0xff;
    int startG = (startValue >> 8) & 0xff;
    int startB = startValue & 0xff;

    int endA = (endValue >> 24) & 0xff;
    int endR = (endValue >> 16) & 0xff;
    int endG = (endValue >> 8) & 0xff;
    int endB = endValue & 0xff;

    // 分离插值
    // 使用 int 运算避免精度问题，最后 clamp 防止溢出
    int currentA = startA + (int)(fraction * (float)(endA - startA));
    int currentR = startR + (int)(fraction * (float)(endR - startR));
    int currentG = startG + (int)(fraction * (float)(endG - startG));
    int currentB = startB + (int)(fraction * (float)(endB - startB));

    // 防御性 Clamping (虽然理论上 fraction 0-1 不会溢出，但在物理回弹动画中 fraction 可能 >1 或 <0)
    currentA = CLAMP(currentA, 0, 255);
    currentR = CLAMP(currentR, 0, 255);
    currentG = CLAMP(currentG, 0, 255);
    currentB = CLAMP(currentB, 0, 255);

    // 合并通道
    return (uint32_t)((currentA << 24) | (currentR << 16) | (currentG << 8) | currentB);
}

// ==========================================
// Point Evaluator
// ==========================================
LFPoint LFPointEvaluator::evaluate(float fraction, const LFPoint& startValue, const LFPoint& endValue) {
    float x = startValue.x + (endValue.x - startValue.x) * fraction;
    float y = startValue.y + (endValue.y - startValue.y) * fraction;
    return LFPoint(x, y);
}

// ==========================================
// Rect Evaluator
// ==========================================
LFRect LFRectEvaluator::evaluate(float fraction, const LFRect& startValue, const LFRect& endValue) {
    float x = startValue.x + (endValue.x - startValue.x) * fraction;
    float y = startValue.y + (endValue.y - startValue.y) * fraction;
    float w = startValue.width + (endValue.width - startValue.width) * fraction;
    float h = startValue.height + (endValue.height - startValue.height) * fraction;
    return LFRect(x, y, w, h);
}

// ==========================================
// Transform Evaluator
// ==========================================
LFTransform LFTransformEvaluator::evaluate(float fraction, const LFTransform& startValue, const LFTransform& endValue) {
    LFTransform result;

    // 线性插值所有属性
    // 注意：对于旋转 (Rotate)，简单的线性插值通常足够 (0->360)
    // 如果需要“最短路径旋转”(例如 350->10 度不应该转一整圈)，需要特殊处理
    // 这里为了通用性暂用线性插值

    result.scaleX = startValue.scaleX + (endValue.scaleX - startValue.scaleX) * fraction;
    result.scaleY = startValue.scaleY + (endValue.scaleY - startValue.scaleY) * fraction;

    result.rotate = startValue.rotate + (endValue.rotate - startValue.rotate) * fraction;

    result.translateX = startValue.translateX + (endValue.translateX - startValue.translateX) * fraction;
    result.translateY = startValue.translateY + (endValue.translateY - startValue.translateY) * fraction;

    result.translatePercentX = startValue.translatePercentX + (endValue.translatePercentX - startValue.translatePercentX) * fraction;
    result.translatePercentY = startValue.translatePercentY + (endValue.translatePercentY - startValue.translatePercentY) * fraction;

    return result;
}
