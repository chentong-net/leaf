//
// Created by Chen Tong on 2026/1/25.
//

#ifndef LEAF_LFSPRINGADAPTER_H
#define LEAF_LFSPRINGADAPTER_H

#include "LFDef.h"
#include "LFSpring.h"
#include "event/LFEvent.h"

// 为了让 LFSpring (Scalar) 能驱动 Vector 类型 (Point, Color)，需要适配器
template <typename T>
class LFSpringAdapter {
public:
    // 默认不支持物理 (比如 String)，编译时报错或空实现
    void init(double mass, double stiff, double damp) {}
    void setTargets(const T& current, const T& target, const T& velocity) {}
    T advance(float dt) { return T(); }
    bool isAtRest() { return true; }
    void setConfig(double dampingRatio, double freq) {}
};

// 特化: Float (单弹簧)
template <>
class LFSpringAdapter<float> {
public:
    void init(double mass, double stiff, double damp) {
        spring.setPhysicalParameters(mass, stiff, damp);
    }
    void setConfig(double dampingRatio, double freq) {
        spring.setDampingRatio(dampingRatio, freq);
    }
    void setTargets(const float& current, const float& target, const float& velocity) {
        spring.setCurrentValue(current, velocity);
        spring.setTargetValue(target);
    }
    float advance(float dt) {
        return (float)spring.advance(dt);
    }
    bool isAtRest() {
        return spring.isAtRest();
    }
private:
    LFSpring spring;
};

// 特化: Point (双弹簧 XY)
template <>
class LFSpringAdapter<LFPoint> {
public:
    void init(double mass, double stiff, double damp) {
        springX.setPhysicalParameters(mass, stiff, damp);
        springY.setPhysicalParameters(mass, stiff, damp);
    }
    void setConfig(double dampingRatio, double freq) {
        springX.setDampingRatio(dampingRatio, freq);
        springY.setDampingRatio(dampingRatio, freq);
    }
    void setTargets(const LFPoint& current, const LFPoint& target, const LFPoint& velocity) {
        springX.setCurrentValue(current.x, velocity.x);
        springX.setTargetValue(target.x);

        springY.setCurrentValue(current.y, velocity.y);
        springY.setTargetValue(target.y);
    }
    LFPoint advance(float dt) {
        float x = (float)springX.advance(dt);
        float y = (float)springY.advance(dt);
        return LFPoint(x, y);
    }
    bool isAtRest() {
        return springX.isAtRest() && springY.isAtRest();
    }
private:
    LFSpring springX;
    LFSpring springY;
};

// 特化: Color (4弹簧 ARGB) - 只有这样才能实现物理级变色
template <>
class LFSpringAdapter<uint32_t> {
public:
    void init(double mass, double stiff, double damp) {
        for(auto& s : springs) s.setPhysicalParameters(mass, stiff, damp);
    }
    void setConfig(double dampingRatio, double freq) {
        for(auto& s : springs) s.setDampingRatio(dampingRatio, freq);
    }
    void setTargets(const uint32_t& current, const uint32_t& target, const uint32_t& velocity) {
        // 解包 ARGB
        double c[4] = {
            (double)((current >> 24) & 0xFF), (double)((current >> 16) & 0xFF),
            (double)((current >> 8) & 0xFF), (double)(current & 0xFF)
        };
        double t[4] = {
            (double)((target >> 24) & 0xFF), (double)((target >> 16) & 0xFF),
            (double)((target >> 8) & 0xFF), (double)(target & 0xFF)
        };
        // 简单假设颜色速度为0
        for(int i=0; i<4; i++) {
            springs[i].setCurrentValue(c[i], 0);
            springs[i].setTargetValue(t[i]);
        }
    }
    uint32_t advance(float dt) {
        int r[4];
        for(int i=0; i<4; i++) {
            r[i] = (int)springs[i].advance(dt);
            if(r[i] < 0) r[i] = 0; if(r[i] > 255) r[i] = 255;
        }
        return (uint32_t)((r[0] << 24) | (r[1] << 16) | (r[2] << 8) | r[3]);
    }
    bool isAtRest() {
        bool rest = true;
        for(auto& s : springs) rest &= s.isAtRest();
        return rest;
    }
private:
    LFSpring springs[4]; // A, R, G, B
};

#endif //LEAF_LFSPRINGADAPTER_H