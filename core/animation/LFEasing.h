//
// Created by Chen Tong on 2026/1/24.
//

#ifndef LEAF_LFEASING_H
#define LEAF_LFEASING_H

#include "LFDef.h"

/**
 * 缓动类型枚举
 * 涵盖了所有标准的 Robert Penner 缓动曲线
 */
enum class LFEasingType {
    Linear,

    // Quadratic (t^2) - 平滑，起步/结束较慢
    QuadIn,
    QuadOut,
    QuadInOut,

    // Cubic (t^3) - 比 Quad 更明显的加速/减速
    CubicIn,
    CubicOut,
    CubicInOut,

    // Quartic (t^4)
    QuartIn,
    QuartOut,
    QuartInOut,

    // Quintic (t^5) - 非常剧烈的加速/减速
    QuintIn,
    QuintOut,
    QuintInOut,

    // Sinusoidal (sin(t)) - 非常柔和的过渡
    SineIn,
    SineOut,
    SineInOut,

    // Circular (sqrt(1-t^2)) - 类似圆弧的轨迹
    CircIn,
    CircOut,
    CircInOut,

    // Exponential (2^t) - 极慢开始，极快结束
    ExpIn,
    ExpOut,
    ExpInOut,

    // Elastic - 弹性 (像被拉伸的橡皮筋)
    // 现代 UI (如 iOS) 常用 EaseOutElastic 做弹窗出现
    ElasticIn,
    ElasticOut,
    ElasticInOut,

    // Back - 回缩 (像拉弓射箭，会超过目标值一点再回来)
    // 适合强调效果
    BackIn,
    BackOut,
    BackInOut,

    // Bounce - 弹跳 (像皮球落地)
    // 适合游戏化 UI
    BounceIn,
    BounceOut,
    BounceInOut
};

class LFEasing {
public:
    /**
     * 计算缓动值
     * @param type 缓动类型
     * @param t 时间进度，范围 [0.0, 1.0]
     * @return 变换后的进度值 (通常在 [0.0, 1.0] 之间，但 Elastic/Back 会超出此范围)
     */
    static float get(LFEasingType type, float t);

private:
    // 禁止实例化
    LFEasing() = default;

    // --- 内部具体算法实现 ---
    // In: 从 0 开始加速
    // Out: 减速并在 1 结束 (UI 最常用)
    // InOut: 前半段加速，后半段减速

    static float EaseInQuad(float t);
    static float EaseOutQuad(float t);
    static float EaseInOutQuad(float t);

    static float EaseInCubic(float t);
    static float EaseOutCubic(float t);
    static float EaseInOutCubic(float t);

    static float EaseInQuart(float t);
    static float EaseOutQuart(float t);
    static float EaseInOutQuart(float t);

    static float EaseInQuint(float t);
    static float EaseOutQuint(float t);
    static float EaseInOutQuint(float t);

    static float EaseInSine(float t);
    static float EaseOutSine(float t);
    static float EaseInOutSine(float t);

    static float EaseInCirc(float t);
    static float EaseOutCirc(float t);
    static float EaseInOutCirc(float t);

    static float EaseInExp(float t);
    static float EaseOutExp(float t);
    static float EaseInOutExp(float t);

    static float EaseInElastic(float t);
    static float EaseOutElastic(float t);
    static float EaseInOutElastic(float t);

    static float EaseInBack(float t);
    static float EaseOutBack(float t);
    static float EaseInOutBack(float t);

    static float EaseInBounce(float t);
    static float EaseOutBounce(float t);
    static float EaseInOutBounce(float t);
};

#endif //LEAF_LFEASING_H