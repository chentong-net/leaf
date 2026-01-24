//
// Created by Chen Tong on 2026/1/24.
//

#include "LFEasing.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 常量定义
// c1, c2, c3 用于 Back 缓动，控制回弹力度 (Overshoot)
const float c1 = 1.70158f;
const float c2 = c1 * 1.525f;
const float c3 = c1 + 1.0f;
// c4, c5 用于 Elastic 缓动，控制周期
const float c4 = (2.0f * (float)M_PI) / 3.0f;
const float c5 = (2.0f * (float)M_PI) / 4.5f;

float LFEasing::get(LFEasingType type, float t) {
    // 通过时间进度计算当前动画进度
    // 边界保护：确保时间严格在 0~1 之间
    // 虽然物理模拟可能超过 1，但标准缓动通常输入需要 clamp
    // (Back/Elastic 的输出值允许超过 1)
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    switch (type) {
        case LFEasingType::Linear:       return t;

        case LFEasingType::QuadIn:       return EaseInQuad(t);
        case LFEasingType::QuadOut:      return EaseOutQuad(t);
        case LFEasingType::QuadInOut:    return EaseInOutQuad(t);

        case LFEasingType::CubicIn:      return EaseInCubic(t);
        case LFEasingType::CubicOut:     return EaseOutCubic(t);
        case LFEasingType::CubicInOut:   return EaseInOutCubic(t);

        case LFEasingType::QuartIn:      return EaseInQuart(t);
        case LFEasingType::QuartOut:     return EaseOutQuart(t);
        case LFEasingType::QuartInOut:   return EaseInOutQuart(t);

        case LFEasingType::QuintIn:      return EaseInQuint(t);
        case LFEasingType::QuintOut:     return EaseOutQuint(t);
        case LFEasingType::QuintInOut:   return EaseInOutQuint(t);

        case LFEasingType::SineIn:       return EaseInSine(t);
        case LFEasingType::SineOut:      return EaseOutSine(t);
        case LFEasingType::SineInOut:    return EaseInOutSine(t);

        case LFEasingType::CircIn:       return EaseInCirc(t);
        case LFEasingType::CircOut:      return EaseOutCirc(t);
        case LFEasingType::CircInOut:    return EaseInOutCirc(t);

        case LFEasingType::ExpIn:        return EaseInExp(t);
        case LFEasingType::ExpOut:       return EaseOutExp(t);
        case LFEasingType::ExpInOut:     return EaseInOutExp(t);

        case LFEasingType::ElasticIn:    return EaseInElastic(t);
        case LFEasingType::ElasticOut:   return EaseOutElastic(t);
        case LFEasingType::ElasticInOut: return EaseInOutElastic(t);

        case LFEasingType::BackIn:       return EaseInBack(t);
        case LFEasingType::BackOut:      return EaseOutBack(t);
        case LFEasingType::BackInOut:    return EaseInOutBack(t);

        case LFEasingType::BounceIn:     return EaseInBounce(t);
        case LFEasingType::BounceOut:    return EaseOutBounce(t);
        case LFEasingType::BounceInOut:  return EaseInOutBounce(t);

        default: return t;
    }
}

// ----------------------------------------------------------------------------
// Implementations
// ----------------------------------------------------------------------------

// Quad
float LFEasing::EaseInQuad(float t) {
    return t * t;
}
float LFEasing::EaseOutQuad(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}
float LFEasing::EaseInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

// Cubic
float LFEasing::EaseInCubic(float t) {
    return t * t * t;
}
float LFEasing::EaseOutCubic(float t) {
    return 1.0f - std::pow(1.0f - t, 3.0f);
}
float LFEasing::EaseInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

// Quart
float LFEasing::EaseInQuart(float t) {
    return t * t * t * t;
}
float LFEasing::EaseOutQuart(float t) {
    return 1.0f - std::pow(1.0f - t, 4.0f);
}
float LFEasing::EaseInOutQuart(float t) {
    return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;
}

// Quint
float LFEasing::EaseInQuint(float t) {
    return t * t * t * t * t;
}
float LFEasing::EaseOutQuint(float t) {
    return 1.0f - std::pow(1.0f - t, 5.0f);
}
float LFEasing::EaseInOutQuint(float t) {
    return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 5.0f) / 2.0f;
}

// Sine
float LFEasing::EaseInSine(float t) {
    return 1.0f - std::cos((t * (float)M_PI) / 2.0f);
}
float LFEasing::EaseOutSine(float t) {
    return std::sin((t * (float)M_PI) / 2.0f);
}
float LFEasing::EaseInOutSine(float t) {
    return -(std::cos((float)M_PI * t) - 1.0f) / 2.0f;
}

// Circ
float LFEasing::EaseInCirc(float t) {
    return 1.0f - std::sqrt(1.0f - std::pow(t, 2.0f));
}
float LFEasing::EaseOutCirc(float t) {
    return std::sqrt(1.0f - std::pow(t - 1.0f, 2.0f));
}
float LFEasing::EaseInOutCirc(float t) {
    return t < 0.5f
           ? (1.0f - std::sqrt(1.0f - std::pow(2.0f * t, 2.0f))) / 2.0f
           : (std::sqrt(1.0f - std::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;
}

// Exp
float LFEasing::EaseInExp(float t) {
    return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f);
}
float LFEasing::EaseOutExp(float t) {
    return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}
float LFEasing::EaseInOutExp(float t) {
    return t == 0.0f
           ? 0.0f
           : t == 1.0f
             ? 1.0f
             : t < 0.5f
               ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f
               : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}

// Elastic
float LFEasing::EaseInElastic(float t) {
    if (t == 0) return 0;
    if (t == 1) return 1;
    return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * c4);
}
float LFEasing::EaseOutElastic(float t) {
    if (t == 0) return 0;
    if (t == 1) return 1;
    // 关键公式：2^(-10t) * sin(...) + 1
    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}
float LFEasing::EaseInOutElastic(float t) {
    if (t == 0) return 0;
    if (t == 1) return 1;
    return t < 0.5f
           ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f
           : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
}

// Back
float LFEasing::EaseInBack(float t) {
    return c3 * t * t * t - c1 * t * t;
}
float LFEasing::EaseOutBack(float t) {
    // 翻转坐标系计算
    float v = t - 1.0f;
    return 1.0f + c3 * std::pow(v, 3.0f) + c1 * std::pow(v, 2.0f);
}
float LFEasing::EaseInOutBack(float t) {
    return t < 0.5f
           ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
           : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
}

// Bounce
float LFEasing::EaseOutBounce(float t) {
    // 经典的 4 段回弹逻辑
    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        float v = t - (1.5f / d1);
        return n1 * v * v + 0.75f;
    } else if (t < 2.5f / d1) {
        float v = t - (2.25f / d1);
        return n1 * v * v + 0.9375f;
    } else {
        float v = t - (2.625f / d1);
        return n1 * v * v + 0.984375f;
    }
}
float LFEasing::EaseInBounce(float t) {
    return 1.0f - EaseOutBounce(1.0f - t);
}
float LFEasing::EaseInOutBounce(float t) {
    return t < 0.5f
           ? (1.0f - EaseOutBounce(1.0f - 2.0f * t)) / 2.0f
           : (1.0f + EaseOutBounce(2.0f * t - 1.0f)) / 2.0f;
}
