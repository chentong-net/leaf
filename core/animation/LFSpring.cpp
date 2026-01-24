//
// Created by Chen Tong on 2026/1/24.
//

#include "LFSpring.h"

LFSpring::LFSpring(double mass, double stiffness, double damping)
    : m_mass(mass)
    , m_stiffness(stiffness)
    , m_damping(damping)
    , m_currentValue(0.0)
    , m_targetValue(0.0)
    , m_velocity(0.0)
{
    // 质量不能为0或负数
    if (m_mass <= 1e-5) m_mass = 1.0;
    recomputeSolverParameters();
}

void LFSpring::setPhysicalParameters(double mass, double stiffness, double damping) {
    if (mass <= 1e-5) mass = 1.0;
    m_mass = mass;
    m_stiffness = stiffness;
    m_damping = damping;
    recomputeSolverParameters();
}

void LFSpring::setDampingRatio(double dampingRatio, double frequencyResponse) {
    // 将设计师参数转换为物理参数
    // 公式参考 Android SpringForce / iOS CASpringAnimation 文档
    // stiffness = (2 * PI / frequencyResponse)^2 * mass
    // damping = 4 * PI * dampingRatio * mass / frequencyResponse

    // 假设 Mass 为 1.0 以简化计算（Spring 的运动只取决于比例）
    m_mass = 1.0;
    double angularFrequency = (2.0 * M_PI) / frequencyResponse; // w0
    m_stiffness = angularFrequency * angularFrequency;          // k = w0^2 * m
    m_damping = 2.0 * dampingRatio * angularFrequency;          // c = 2 * zeta * w0 * m

    recomputeSolverParameters();
}

void LFSpring::recomputeSolverParameters() {
    // w0 = sqrt(k / m)
    m_solverParams.w0 = std::sqrt(m_stiffness / m_mass);
    // zeta = c / (2 * sqrt(k * m)) = c / (2 * m * w0)
    m_solverParams.zeta = m_damping / (2.0 * std::sqrt(m_stiffness * m_mass));

    if (m_solverParams.zeta < 1.0) {
        // 欠阻尼 (Under-damped)
        // wd = w0 * sqrt(1 - zeta^2)
        m_solverParams.wd = m_solverParams.w0 * std::sqrt(1.0 - m_solverParams.zeta * m_solverParams.zeta);
    } else if (m_solverParams.zeta > 1.0) {
        // 过阻尼 (Over-damped)
        // r1, r2 = -w0 * (zeta +/- sqrt(zeta^2 - 1))
        double sqrtPart = std::sqrt(m_solverParams.zeta * m_solverParams.zeta - 1.0);
        m_solverParams.r1 = -m_solverParams.w0 * (m_solverParams.zeta - sqrtPart);
        m_solverParams.r2 = -m_solverParams.w0 * (m_solverParams.zeta + sqrtPart);
    } else {
        // 临界阻尼 (Critically damped): zeta == 1.0
        // 不需要额外参数，r = -w0
    }
}

void LFSpring::setCurrentValue(double value, double velocity) {
    m_currentValue = value;
    m_velocity = velocity;
}

void LFSpring::setTargetValue(double target) {
    if (m_targetValue == target) return;
    m_targetValue = target;
    // 目标改变时，不重置 Velocity，利用惯性自然过渡
}

void LFSpring::snapToTarget() {
    m_currentValue = m_targetValue;
    m_velocity = 0.0;
}

void LFSpring::setRestTolerance(double displacementTolerance, double velocityTolerance) {
    m_restDisplacementTolerance = displacementTolerance;
    m_restVelocityTolerance = velocityTolerance;
}

bool LFSpring::isAtRest() const {
    return std::abs(m_velocity) <= m_restVelocityTolerance &&
           std::abs(m_currentValue - m_targetValue) <= m_restDisplacementTolerance;
}

double LFSpring::advance(double dt) {
    if (isAtRest()) {
        m_currentValue = m_targetValue;
        m_velocity = 0.0;
        return m_currentValue;
    }

    // 为了避免浮点数精度问题导致的 dt=0
    if (dt <= 1e-5) return m_currentValue;

    // 限制最大单步步长，防止极低帧率下物理模拟炸飞
    // 如果系统卡顿导致 dt 很大（例如 500ms），我们应该分步积分或者限制它
    // 但解析解法其实不怕大 dt，这里主要为了防止极端数学行为
    const double MAX_DT = 0.1; // 100ms
    if (dt > MAX_DT) dt = MAX_DT;

    // 计算相对于平衡位置的位移 (x0) 和速度 (v0)
    // y(t) = m_currentValue - target
    double x0 = m_currentValue - m_targetValue;
    double v0 = m_velocity;

    double offsetFinal = 0.0;   // t 时刻的位移 y(t)
    double velocityFinal = 0.0; // t 时刻的速度 v(t)

    // 根据阻尼比选择不同的解析解公式
    if (m_solverParams.zeta < 1.0) {
        // ==========================================
        // 1. 欠阻尼 (Under-damped) - 发生震荡
        // y(t) = e^(-zeta * w0 * t) * (c1 * cos(wd * t) + c2 * sin(wd * t))
        // ==========================================
        double envelope = std::exp(-m_solverParams.zeta * m_solverParams.w0 * dt);

        double c1 = x0;
        double c2 = (v0 + m_solverParams.zeta * m_solverParams.w0 * x0) / m_solverParams.wd;

        double cosPart = std::cos(m_solverParams.wd * dt);
        double sinPart = std::sin(m_solverParams.wd * dt);

        offsetFinal = envelope * (c1 * cosPart + c2 * sinPart);

        // 计算速度 (对 y(t) 求导)
        // v(t) = ... (详细推导省略，直接用结果)
        velocityFinal = envelope * ((c2 * m_solverParams.wd - c1 * m_solverParams.zeta * m_solverParams.w0) * cosPart -
                                    (c1 * m_solverParams.wd + c2 * m_solverParams.zeta * m_solverParams.w0) * sinPart);

    } else if (m_solverParams.zeta > 1.0) {
        // ==========================================
        // 2. 过阻尼 (Over-damped) - 缓慢逼近，无震荡
        // y(t) = c1 * e^(r1 * t) + c2 * e^(r2 * t)
        // ==========================================
        double c2 = (v0 - m_solverParams.r1 * x0) / (m_solverParams.r2 - m_solverParams.r1);
        double c1 = x0 - c2;

        double exp1 = std::exp(m_solverParams.r1 * dt);
        double exp2 = std::exp(m_solverParams.r2 * dt);

        offsetFinal = c1 * exp1 + c2 * exp2;
        velocityFinal = c1 * m_solverParams.r1 * exp1 + c2 * m_solverParams.r2 * exp2;

    } else {
        // ==========================================
        // 3. 临界阻尼 (Critically damped) - 最快逼近
        // y(t) = e^(-w0 * t) * (c1 + c2 * t)
        // ==========================================
        double envelope = std::exp(-m_solverParams.w0 * dt);
        double c1 = x0;
        double c2 = v0 + m_solverParams.w0 * x0;

        offsetFinal = envelope * (c1 + c2 * dt);
        velocityFinal = envelope * (c2 - m_solverParams.w0 * (c1 + c2 * dt));
    }

    // 更新状态
    m_currentValue = m_targetValue + offsetFinal;
    m_velocity = velocityFinal;

    return m_currentValue;
}
