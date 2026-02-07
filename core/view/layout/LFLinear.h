//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFLINEAR_H
#define LEAF_LFLINEAR_H

#include "view/base/LFNode.h"

enum class LFOrientation { Vertical, Horizontal };
enum class LFAlignment { Start, Center, End, Stretch, Baseline };
enum class LFDistribution { Pack, SpaceBetween, SpaceAround, SpaceEvenly };

/**
 * 线性布局
 */
class LFLinear : public LFNode {
public:
    LFLinear();
    virtual ~LFLinear() = default;

    void setOrientation(LFOrientation orientation);
    void setGravity(LFAlignment main, LFAlignment cross);
    void setDistribution(LFDistribution distribution);
    void setSpacing(float spacing);
    void setReverse(bool reverse); // 反转布局

    // 工厂方法
    static std::shared_ptr<LFLinear> createVertical();
    static std::shared_ptr<LFLinear> createHorizontal();

private:
    // 核心逻辑：根据语义状态更新 Yoga 属性
    void updateYogaLayout();

    LFOrientation m_orientation = LFOrientation::Vertical;
    LFAlignment m_mainAlign = LFAlignment::Start;
    LFAlignment m_crossAlign = LFAlignment::Stretch;
    LFDistribution m_distribution = LFDistribution::Pack;
    bool m_isReverse = false;
};

#endif // LEAF_LFLINEAR_H