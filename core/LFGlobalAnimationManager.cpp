//
// Created by Chen Tong on 2026/1/25.
//

//
// Created by Leaf Engine Team.
//

#include "LFGlobalAnimationManager.h"

void LFGlobalAnimationManager::addAnimator(LFAnimator::Ptr animator) {
    if (!animator) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    // 简单查重
    auto it = std::find(m_animators.begin(), m_animators.end(), animator);
    if (it == m_animators.end()) {
        m_animators.push_back(animator);
    }
}

void LFGlobalAnimationManager::removeAnimator(LFAnimator::Ptr animator) {
    if (!animator) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find(m_animators.begin(), m_animators.end(), animator);
    if (it != m_animators.end()) {
        m_animators.erase(it);
    }
}

void LFGlobalAnimationManager::update(float dt) {
    std::vector<LFAnimator::Ptr> activeList;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_animators.empty()) return;
        // 拷贝列表，防止在 tick 回调中 add/remove 动画导致 Crash
        activeList = m_animators;
    }

    // 遍历执行。已在本帧开始后移除的动画不再推进。
    for (auto& anim : activeList) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (std::find(m_animators.begin(), m_animators.end(), anim) == m_animators.end()) {
                continue;
            }
        }
        if (anim && anim->isRunning()) {
            anim->tick(dt);
        }
    }

    // 清理已结束的动画
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::remove_if(m_animators.begin(), m_animators.end(),
         [](const LFAnimator::Ptr& anim) {
             if (!anim) return true;
             auto state = anim->getState();
             // 移除 结束、停止、取消 或 还没开始(Created)但不应该在运行列表里的
             return state == LFAnimatorState::Ended ||
                    state == LFAnimatorState::Stopped ||
                    state == LFAnimatorState::Cancelled;
         });

    if (it != m_animators.end()) {
        m_animators.erase(it, m_animators.end());
    }
}
