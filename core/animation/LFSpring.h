//
// Created by Chen Tong on 2026/1/24.
//

#ifndef LEAF_LFSPRING_H
#define LEAF_LFSPRING_H

#include "LFDef.h"

/**
 * 物理弹簧模拟器
 * 基于阻尼谐振子 (Damped Harmonic Oscillator) 模型。
 *
 * 核心公式:
 * m * d^2x/dt^2 + c * dx/dt + k * (x - target) = 0
 *
 * 相比于 Easing 曲线，Spring 的优势在于：
 * 1. 连续性：当目标值在中途改变时，Spring 会保留当前的速度（惯性），产生自然的过渡，而 Easing 通常会生硬地重置。
 * 2. 真实感：基于物理定律，符合人类对物体运动的直觉。
 */
class LFSpring {
public:
    /**
     * 构造函数
     * @param mass 质量 (默认为 1.0)
     * @param stiffness 刚度 (决定弹性/速度，值越大越快)
     * @param damping 阻尼 (决定回弹/刹车力度，值越小回弹越多)
     */
    LFSpring(double mass = 1.0, double stiffness = 100.0, double damping = 10.0);

    virtual ~LFSpring() = default;

    // ==========================================
    // 配置接口 (Configuration)
    // ==========================================

    /**
     * 设置物理参数 (专业模式)
     */
    void setPhysicalParameters(double mass, double stiffness, double damping);

    /**
     * 设置设计师友好参数 (直观模式)
     * @param dampingRatio 阻尼比 (Zeta)
     * 0.0 = 无阻尼 (永远震荡)
     * 0.0~1.0 = 欠阻尼 (回弹/OverShoot) -> UI 最常用，推荐 0.5~0.8
     * 1.0 = 临界阻尼 (无回弹，最快稳定)
     * >1.0 = 过阻尼 (无回弹，缓慢逼近)
     * @param frequencyResponse 频率响应 (类似刚度，值越小响应越快/越硬)
     * 推荐 0.5 ~ 1.0 之间
     */
    void setDampingRatio(double dampingRatio, double frequencyResponse = 1.0);

    // ==========================================
    // 状态控制 (State Control)
    // ==========================================

    /**
     * 设置当前状态
     * @param value 当前值
     * @param velocity 当前速度 (默认为 0)
     */
    void setCurrentValue(double value, double velocity = 0.0);

    /**
     * 设置目标值
     * 如果目标值改变，弹簧将开始向新目标运动
     */
    void setTargetValue(double target);

    /**
     * 将当前值强制设为目标值（瞬间归位）
     * 停止运动
     */
    void snapToTarget();

    // ==========================================
    // 核心计算 (Simulation)
    // ==========================================

    /**
     * 向前推进时间
     * @param dt Delta Time (秒)
     * @return 当前最新的值
     */
    double advance(double dt);

    // ==========================================
    // 查询 (Getters)
    // ==========================================

    double getCurrentValue() const { return m_currentValue; }
    double getTargetValue() const { return m_targetValue; }
    double getVelocity() const { return m_velocity; }

    /**
     * 是否已静止
     * 当位置足够接近目标且速度极小时返回 true
     */
    bool isAtRest() const;

    /**
     * 设置静止容差
     * @param displacementTolerance 位移容差 (例如 0.001)
     * @param velocityTolerance 速度容差 (例如 0.001)
     */
    void setRestTolerance(double displacementTolerance, double velocityTolerance);

private:
    // 物理参数
    double m_mass;
    double m_stiffness;
    double m_damping;

    // 状态变量
    double m_currentValue;
    double m_targetValue;
    double m_velocity;

    // 缓存的导出参数 (避免每帧重复计算 sqrt/pow)
    // 每次物理参数变更时重新计算
    struct SolverParameters {
        double w0;    // 自然角频率 (Natural angular frequency)
        double zeta;  // 阻尼比 (Damping ratio)
        double wd;    // 阻尼角频率 (Damped angular frequency, only for under-damped)
        // 临界阻尼或过阻尼所需的根
        double r1;
        double r2;
    } m_solverParams;

    // 静止阈值
    double m_restDisplacementTolerance = 1e-3;
    double m_restVelocityTolerance = 1e-3;

    // 内部方法：重新计算 Solver 参数
    void recomputeSolverParameters();
};

#endif //LEAF_LFSPRING_H