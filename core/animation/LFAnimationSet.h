//
// Created by Chen Tong on 2026/1/24.
//

#ifndef LEAF_LFANIMATIONSET_H
#define LEAF_LFANIMATIONSET_H

#include "LFAnimator.h"

/**
 * 动画集合
 * 用于编排多个动画的播放顺序（并行或串行）
 */
class LFAnimationSet : public LFAnimator {
public:
    using Ptr = std::shared_ptr<LFAnimationSet>;

    LFAnimationSet() = default;

    /**
     * 添加子动画
     */
    void addAnimator(LFAnimator::Ptr animator);

    /**
     * 批量添加 (C++11 initializer list)
     * 用法: set->playTogether({anim1, anim2, anim3});
     */
    void playTogether(std::initializer_list<LFAnimator::Ptr> items);
    void playSequentially(std::initializer_list<LFAnimator::Ptr> items);

    // 重写生命周期
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void cancel() override;

    // 核心 Tick
    bool tick(float dt) override;

private:
    bool m_playSequentially = false; // 默认为并行 (Together)

    std::vector<LFAnimator::Ptr> m_animators;

    // 串行播放时的状态
    size_t m_currentIndex = 0;
    bool m_currentStarted = false;
};

#endif //LEAF_LFANIMATIONSET_H