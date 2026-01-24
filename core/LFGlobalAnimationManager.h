//
// Created by Chen Tong on 2026/1/25.
//

#ifndef LEAF_LFGLOBALANIMATIONMANAGER_H
#define LEAF_LFGLOBALANIMATIONMANAGER_H

#include "LFDef.h"
#include "animation/LFAnimator.h"

/**
 * 全局动画管理器
 * 统一管理所有活跃的 Animator，每一帧更新它们的状态
 */
class LFGlobalAnimationManager {
public:
    static LFGlobalAnimationManager& getInstance() {
        static LFGlobalAnimationManager instance;
        return instance;
    }

    // 禁止拷贝
    LFGlobalAnimationManager(const LFGlobalAnimationManager&) = delete;
    LFGlobalAnimationManager& operator=(const LFGlobalAnimationManager&) = delete;

    /**
     * 注册动画
     */
    void addAnimator(LFAnimator::Ptr animator);

    /**
     * 移除动画
     */
    void removeAnimator(LFAnimator::Ptr animator);

    /**
     * 驱动所有动画 (由 Engine 每帧调用)
     * @param dt Delta Time (Seconds)
     */
    void update(float dt);

private:
    LFGlobalAnimationManager() = default;

    std::vector<LFAnimator::Ptr> m_animators;
    // 如果你的应用涉及多线程添加动画，建议加锁；如果都在主线程，可不加。
    // TODO: 为了稳健，这里预留 Mutex 思路，暂不实现复杂锁逻辑。
};

#endif //LEAF_LFGLOBALANIMATIONMANAGER_H