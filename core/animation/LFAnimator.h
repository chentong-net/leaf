//
// Created by Chen Tong on 2026/1/24.
//

#ifndef LEAF_LFANIMATOR_H
#define LEAF_LFANIMATOR_H

#include "LFEasing.h"
#include "LFEvaluator.h"
#include "LFSpring.h"
#include "LFSpringAdapter.h"

enum class LFAnimatorState {
    Created,
    Running,
    Paused,
    Stopped,
    Ended,
    Cancelled
};

/**
 * 动画循环模式
 */
enum class LFAnimRepeatMode {
    Restart,    // 重新开始 (0 -> 1, 0 -> 1)
    Reverse     // 倒放 (0 -> 1, 1 -> 0)
};

/**
 * 动画基类 (非模板)
 * 用于 LFEngine 统一调度
 */
class LFAnimator : public std::enable_shared_from_this<LFAnimator> {
public:
    using Ptr = std::shared_ptr<LFAnimator>;
    using VoidCallback = std::function<void()>;

    LFAnimator();
    virtual ~LFAnimator() = default;

    // =========================
    // 生命周期控制
    // =========================
    virtual void start();
    virtual void stop();
    virtual void cancel();
    virtual void pause();
    virtual void resume();

    // 引擎每帧调用
    // 返回 true 表示动画已结束，可以移除
    virtual bool tick(float dt) = 0;

    // =========================
    // 通用配置
    // =========================
    void setStartDelay(float delaySeconds);
    void setDuration(float durationSeconds); // 仅在 Timing 模式有效

    // 循环控制 (仅 Timing 模式)
    void setLoopCount(int count); // -1 为无限循环
    void setRepeatMode(LFAnimRepeatMode mode);

    // 回调
    void setOnStart(VoidCallback cb) { m_onStart = cb; }
    void setOnEnd(VoidCallback cb) { m_onEnd = cb; }
    void setOnCancel(VoidCallback cb) { m_onCancel = cb; }
    void setOnRepeat(VoidCallback cb) { m_onRepeat = cb; }

    LFAnimatorState getState() const { return m_state; }
    bool isRunning() const { return m_state == LFAnimatorState::Running; }
    bool isPaused() const { return m_state == LFAnimatorState::Paused; }

protected:
    // 状态
    LFAnimatorState m_state = LFAnimatorState::Created;

    // 时间参数
    float m_duration = 0.3f;    // 默认 300ms
    float m_startDelay = 0.0f;
    float m_elapsedTime = 0.0f; // 当前周期内的时间
    float m_totalTime = 0.0f;   // 总运行时间
    float m_delayTimer = 0.0f;

    // 循环状态
    int m_loopCount = 0;        // 0 表示不循环 (共播放1次)
    int m_currentLoop = 0;
    LFAnimRepeatMode m_repeatMode = LFAnimRepeatMode::Restart;
    bool m_isReversing = false; // 当前是否处于 PingPong 的反向阶段

    // 回调
    VoidCallback m_onStart;
    VoidCallback m_onEnd;
    VoidCallback m_onCancel;
    VoidCallback m_onRepeat;

    // 内部生命周期钩子
    void notifyStart();
    void notifyEnd();
    void notifyCancel();
    void notifyRepeat();
};

// =================================================================================
// Value Animator (Templated)
// 核心实现类
// =================================================================================

template <typename T>
class LFValueAnimator : public LFAnimator {
public:
    using UpdateCallback = std::function<void(const T& value)>;
    using Ptr = std::shared_ptr<LFValueAnimator<T>>;

    // 工厂方法
    static Ptr of(T start, T end) {
        auto anim = std::make_shared<LFValueAnimator<T>>();
        anim->setValues(start, end);
        return anim;
    }

    LFValueAnimator() {
        // 默认估值器
        if constexpr (std::is_same<T, float>::value) {
            m_evaluator = std::make_shared<LFFloatEvaluator>();
        } else if constexpr (std::is_same<T, int>::value) {
            m_evaluator = std::make_shared<LFIntEvaluator>();
        } else if constexpr (std::is_same<T, uint32_t>::value) {
            m_evaluator = std::make_shared<LFColorEvaluator>();
        } else if constexpr (std::is_same<T, LFPoint>::value) {
            m_evaluator = std::make_shared<LFPointEvaluator>();
        } else if constexpr (std::is_same<T, LFRect>::value) {
            m_evaluator = std::make_shared<LFRectEvaluator>();
        } else if constexpr (std::is_same<T, LFTransform>::value) {
            m_evaluator = std::make_shared<LFTransformEvaluator>();
        }
    }

    // 设置关键值
    void setValues(T start, T end) {
        m_startValue = start;
        m_endValue = end;
        m_currentValue = start;
    }

    // 设置回调
    void addUpdateListener(UpdateCallback cb) { m_onUpdate = cb; }

    // --- 模式 1: Easing Mode (默认) ---
    void setEasing(LFEasingType type) { m_easingType = type; m_usePhysics = false; }
    void setEvaluator(std::shared_ptr<LFEvaluator<T>> evaluator) { m_evaluator = evaluator; }

    // --- 模式 2: Physics Mode ---
    // 开启物理模式后，Duration 和 Easing 失效
    void setSpring(double dampingRatio = 0.5, double frequencyResponse = 0.8) {
        m_usePhysics = true;
        m_springAdapter.setConfig(dampingRatio, frequencyResponse);
    }

    // 重写 tick 核心逻辑
    bool tick(float dt) override {
        if (m_state != LFAnimatorState::Running) return false;

        // 1. 处理启动延时
        if (m_delayTimer < m_startDelay) {
            m_delayTimer += dt;
            return false;
        }

        // 2. 物理模式逻辑
        if (m_usePhysics) {
            // 物理模式下没有 Duration 概念，只看是否静止
            m_currentValue = m_springAdapter.advance(dt);

            if (m_onUpdate) m_onUpdate(m_currentValue);

            if (m_springAdapter.isAtRest()) {
                // 确保最终值精确
                if (m_onUpdate) m_onUpdate(m_endValue); // 物理结束通常就是到达目标
                notifyEnd();
                m_state = LFAnimatorState::Ended;
                return true;
            }
            return false;
        }

        // 3. Easing 模式逻辑
        m_elapsedTime += dt;
        m_totalTime += dt;

        float fraction = m_elapsedTime / m_duration;
        bool cycleEnded = false;

        // 处理完成或循环
        if (fraction >= 1.0f) {
            if (m_loopCount == -1 || m_currentLoop < m_loopCount) {
                // 进入下一次循环
                m_currentLoop++;
                m_elapsedTime = 0.0f; // 或者 fmod 保留余数
                cycleEnded = true;

                if (m_repeatMode == LFAnimRepeatMode::Reverse) {
                    m_isReversing = !m_isReversing;
                }

                notifyRepeat();
            } else {
                // 彻底结束
                fraction = 1.0f;
                notifyEnd();
                m_state = LFAnimatorState::Ended;
            }
        }

        // 计算缓动进度
        float interpolation;
        if (m_isReversing) {
            // 倒放阶段：进度从 1 -> 0
            // 注意：Easing 函数通常输入 0->1。倒放意味着时间倒流。
            // interpolation = get(1 - fraction)
            interpolation = LFEasing::get(m_easingType, 1.0f - fraction);
        } else {
            interpolation = LFEasing::get(m_easingType, fraction);
        }

        // 计算具体值
        if (m_evaluator) {
            auto eval = std::static_pointer_cast<LFEvaluator<T>>(m_evaluator);
            m_currentValue = eval->evaluate(interpolation, m_startValue, m_endValue);
        }

        if (m_onUpdate) m_onUpdate(m_currentValue);

        return m_state == LFAnimatorState::Ended;
    }

    void start() override {
        // 初始化状态
        m_elapsedTime = 0.0f;
        m_totalTime = 0.0f;
        m_delayTimer = 0.0f;
        m_currentLoop = 0;
        m_isReversing = false;

        if (m_usePhysics) {
            // 初始化物理引擎
            m_springAdapter.setTargets(m_startValue, m_endValue, T());
        }

        // 调用基类设置状态
        LFAnimator::start();

        // 初始帧回调
        if (m_onUpdate && m_startDelay == 0) m_onUpdate(m_startValue);
    }

private:
    T m_startValue;
    T m_endValue;
    T m_currentValue;

    UpdateCallback m_onUpdate;

    // Easing 模式组件
    LFEasingType m_easingType = LFEasingType::QuadOut;
    std::shared_ptr<void> m_evaluator; // 使用 void* 存储，使用时转回 LFEvaluator<T>

    // Physics 模式组件
    bool m_usePhysics = false;
    LFSpringAdapter<T> m_springAdapter;
};

#endif //LEAF_LFANIMATOR_H