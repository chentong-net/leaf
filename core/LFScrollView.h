//
// Created by Chen Tong on 2026/1/26.
//

#ifndef LEAF_LFSCROLLVIEW_H
#define LEAF_LFSCROLLVIEW_H

#include "LFDef.h"
#include "LFBox.h"
#include "animation/LFAnimator.h"
#include "gesture/LFGestureRecognizer.h"

/**
 * 滚动容器
 */
class LFScrollView : public LFBox {
public:
    using Ptr = std::shared_ptr<LFScrollView>;
    static Ptr createVertical();

    LFScrollView();
    virtual ~LFScrollView();

    /**
     * 重写 addChild
     * 外部调用的 addChild 实际上是添加到内部的 m_content 容器中
     */
    void addChild(const LFNode::Ptr& child); // 覆盖基类
    // 如果需要往 ScrollView 自身添加（比如悬浮按钮），可以用这个
    void addOverlayChild(const LFNode::Ptr& child, LFBoxAlign align = LFBoxAlign::TopLeft, float offsetX = 0.0, float offsetY = 0.0);

    /**
     * 滚动到指定位置
     * @param y 偏移量 (通常是负数或0，0表示顶部)
     * @param animate 是否开启动画
     */
    void scrollTo(float y, bool animate = true);

    float getScrollY() const { return m_scrollY; }

    void setScrollBarEnabled(bool enabled);
    bool isScrollBarEnabled() const { return m_scrollBarEnabled; }

    void setBounces(bool bounces);
    bool getBounces() const { return m_bounces; }

protected:
    // 绘制滚动条
    void onDrawOverlay(NVGcontext* vg) override;

private:
    void initLayout();
    void initGestures();
    void initAnimator();

    // 核心物理引擎 Tick
    void updatePhysics(float dt);

    // 边界检查与修正
    // 返回值: 目标修正位置（如果不需要修正则返回当前位置）
    float getBoundaryY(float targetY) const;
    bool isOutOfBounds(float y) const;

    // 内部容器，承载所有子节点
    std::shared_ptr<LFBox> m_content;

    // --- 滚动状态 ---
    float m_scrollY = 0.0f;       // 当前滚动位置 (通常 <= 0)
    float m_velocity = 0.0f;      // 当前速度 (像素/秒)
    bool m_isDragging = false;    // 是否正在被手指拖拽

    // --- 物理参数 ---
    const float FRICTION = 0.92f; // 摩擦系数 (0.0~1.0)
    const float MIN_VELOCITY = 10.0f; // 最小速度阈值

    // --- 动画控制器 ---
    // 我们使用一个自定义 Animator 来驱动 ScrollView 的物理 tick
    class ScrollPhysicsAnimator : public LFAnimator {
    public:
        using UpdateCallback = std::function<bool(float dt)>;
        void setUpdateCallback(UpdateCallback cb) { m_callback = cb; }
        // 覆盖 tick，不使用标准 Easing，而是由回调决定是否结束
        bool tick(float dt) override {
            if (m_callback) return m_callback(dt);
            return true;
        }
    private:
        UpdateCallback m_callback;
    };
    std::shared_ptr<ScrollPhysicsAnimator> m_animator;

    // 弹簧模拟器 (用于回弹)
    LFSpring m_spring;
    bool m_inSpringMode = false; // 当前是否处于回弹接管状态

    // --- 滚动条相关 ---
    float m_scrollbarOpacity = 0.0f;
    std::shared_ptr<LFValueAnimator<float>> m_barFadeAnimator;
    void showScrollbar();
    void hideScrollbar();

    bool m_scrollBarEnabled = true;

    bool m_bounces = true;
};

#endif // LEAF_LFSCROLLVIEW_H
