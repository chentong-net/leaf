//
// Created by Chen Tong on 2026/1/24.
//

#include "LFAnimationSet.h"

void LFAnimationSet::addAnimator(LFAnimator::Ptr animator) {
    if (animator) {
        m_animators.push_back(animator);
    }
}

void LFAnimationSet::playTogether(std::initializer_list<LFAnimator::Ptr> items) {
    m_playSequentially = false;
    m_animators = items;
}

void LFAnimationSet::playSequentially(std::initializer_list<LFAnimator::Ptr> items) {
    m_playSequentially = true;
    m_animators = items;
}

void LFAnimationSet::start() {
    if (m_animators.empty()) {
        m_state = LFAnimatorState::Ended;
        notifyEnd();
        return;
    }

    // 重置状态
    m_currentIndex = 0;
    m_currentStarted = false;

    // 并行模式：所有子动画预备
    // 注意：我们不在这里立即调用子动画的 start()，
    // 而是在第一次 tick 时调用，或者在这里调用。
    // 为了逻辑统一，我们在 start 里调用。
    if (!m_playSequentially) {
        for (auto& anim : m_animators) {
            // 重置子动画状态，防止复用时状态不对
            // 但 LFAnimator 没有 reset()？通常创建新的比较好。
            // 如果必须复用，确保它们不是 Running 状态
            anim->start();
        }
    } else {
        // 串行模式：只启动第一个
        // 逻辑在 tick 里处理更连贯
    }

    LFAnimator::start(); // 设置状态为 Running 并触发 onStart
}

void LFAnimationSet::stop() {
    LFAnimator::stop();
    for (auto& anim : m_animators) {
        anim->stop();
    }
}

void LFAnimationSet::pause() {
    LFAnimator::pause();
    for (auto& anim : m_animators) {
        anim->pause();
    }
}

void LFAnimationSet::resume() {
    LFAnimator::resume();
    for (auto& anim : m_animators) {
        anim->resume();
    }
}

void LFAnimationSet::cancel() {
    LFAnimator::cancel();
    for (auto& anim : m_animators) {
        anim->cancel();
    }
}

bool LFAnimationSet::tick(float dt) {
    if (m_state != LFAnimatorState::Running) return false;

    // 1. 处理自身的 StartDelay (基类逻辑)
    if (m_delayTimer < m_startDelay) {
        m_delayTimer += dt;
        return false;
    }

    // 2. 驱动子动画
    bool allFinished = true;

    if (m_playSequentially) {
        // === 串行模式 ===
        if (m_currentIndex < m_animators.size()) {
            auto currentAnim = m_animators[m_currentIndex];

            // 首次进入当前动画，启动它
            if (!m_currentStarted) {
                currentAnim->start();
                m_currentStarted = true;
            }

            // 驱动当前动画
            bool finished = currentAnim->tick(dt);

            // 如果当前动画结束
            if (finished) {
                // 移动到下一个
                m_currentIndex++;
                m_currentStarted = false;

                // 如果还有下一个，本帧不结束，或者本帧 Set 还没结束
                if (m_currentIndex < m_animators.size()) {
                    allFinished = false;
                }
            } else {
                // 当前没结束，整体肯定没结束
                allFinished = false;
            }
        }
    } else {
        // === 并行模式 ===
        for (auto& anim : m_animators) {
            // 只有正在运行的才 tick
            if (anim->isRunning()) {
                bool finished = anim->tick(dt);
                if (!finished) {
                    allFinished = false; // 只要有一个没结束，整体就没结束
                }
            } else if (anim->getState() != LFAnimatorState::Ended) {
                // 这种防御性代码：如果子动画还没 Start?
                // 我们在 Set::start() 里已经全启动了，所以这里通常不会进
            }
        }
    }

    if (allFinished) {
        // 处理 Set 自身的循环逻辑 (Loop)
        // Set 的循环意味着：重置所有子动画，从头再来
        // 这部分逻辑比较复杂，通常 Set 不建议 Loop。
        // 如果需要 Loop，简单处理：

        // 目前简单实现：直接结束
        m_state = LFAnimatorState::Ended;
        notifyEnd();
        return true;
    }

    return false;
}
