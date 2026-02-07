//
// Created by Chen Tong on 2026/1/24.
//

#ifndef LEAF_LFEVALUATOR_H
#define LEAF_LFEVALUATOR_H

#include "event/LFEvent.h"  // For LFPoint, LFRect
#include "view/base/LFNode.h"         // For LFTransform

/**
 * 估值器接口
 * 根据进度计算当前的值: result = start + (end - start) * fraction
 */
template <typename T>
class LFEvaluator {
public:
    virtual ~LFEvaluator() = default;
    virtual T evaluate(float fraction, const T& startValue, const T& endValue) = 0;
};

/**
 * 浮点数估值器
 */
class LFFloatEvaluator : public LFEvaluator<float> {
public:
    float evaluate(float fraction, const float& startValue, const float& endValue) override {
        return startValue + fraction * (endValue - startValue);
    }
};

/**
 * 整数估值器
 * 结果会取整
 */
class LFIntEvaluator : public LFEvaluator<int> {
public:
    int evaluate(float fraction, const int& startValue, const int& endValue) override {
        return startValue + (int)(fraction * (float)(endValue - startValue));
    }
};

/**
 * 颜色估值器 (ARGB)
 * 关键类：不能直接对 uint32 做线性运算，否则颜色会发灰或出现杂色。
 * 必须将 ARGB 四个通道分离，分别插值后再合并。
 */
class LFColorEvaluator : public LFEvaluator<uint32_t> {
public:
    // 实现位于 .cpp 以保持头文件整洁
    uint32_t evaluate(float fraction, const uint32_t& startValue, const uint32_t& endValue) override;
};

/**
 * 点估值器 (LFPoint)
 * 用于位移动画
 */
class LFPointEvaluator : public LFEvaluator<LFPoint> {
public:
    LFPoint evaluate(float fraction, const LFPoint& startValue, const LFPoint& endValue) override;
};

/**
 * 矩形估值器 (LFRect)
 * 用于 Frame 动画 (位置+大小同时变化)
 */
class LFRectEvaluator : public LFEvaluator<LFRect> {
public:
    LFRect evaluate(float fraction, const LFRect& startValue, const LFRect& endValue) override;
};

/**
 * 变换估值器 (LFTransform)
 * 高级功能：用于复杂的形变动画 (平移/旋转/缩放同时进行)
 */
class LFTransformEvaluator : public LFEvaluator<LFTransform> {
public:
    LFTransform evaluate(float fraction, const LFTransform& startValue, const LFTransform& endValue) override;
};

#endif //LEAF_LFEVALUATOR_H