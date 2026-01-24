//
// Created by Chen Tong on 2026/1/24.
//

//
// Created by Leaf Engine Team.
// Animation System - Core Animator Implementation
//

#include "LFAnimator.h"

LFAnimator::LFAnimator() {

}

void LFAnimator::start() {
    if (m_state == LFAnimatorState::Running) return;
    m_state = LFAnimatorState::Running;
    notifyStart();
}

void LFAnimator::stop() {
    if (m_state == LFAnimatorState::Stopped || m_state == LFAnimatorState::Created) return;
    m_state = LFAnimatorState::Stopped;
    // stop 不触发 onEnd，只触发 onCancel? 或者都不触发
    // 通常 stop 意味着强制停止
}

void LFAnimator::cancel() {
    if (m_state == LFAnimatorState::Running || m_state == LFAnimatorState::Paused) {
        m_state = LFAnimatorState::Cancelled;
        notifyCancel();
    }
}

void LFAnimator::pause() {
    if (m_state == LFAnimatorState::Running) {
        m_state = LFAnimatorState::Paused;
    }
}

void LFAnimator::resume() {
    if (m_state == LFAnimatorState::Paused) {
        m_state = LFAnimatorState::Running;
    }
}

void LFAnimator::setStartDelay(float delaySeconds) {
    m_startDelay = delaySeconds;
}

void LFAnimator::setDuration(float durationSeconds) {
    if (durationSeconds < 0) durationSeconds = 0;
    m_duration = durationSeconds;
}

void LFAnimator::setLoopCount(int count) {
    m_loopCount = count;
}

void LFAnimator::setRepeatMode(LFAnimRepeatMode mode) {
    m_repeatMode = mode;
}

// =========================
// 通知辅助函数
// =========================

void LFAnimator::notifyStart() {
    if (m_onStart) m_onStart();
}

void LFAnimator::notifyEnd() {
    if (m_onEnd) m_onEnd();
}

void LFAnimator::notifyCancel() {
    if (m_onCancel) m_onCancel();
}

void LFAnimator::notifyRepeat() {
    if (m_onRepeat) m_onRepeat();
}
