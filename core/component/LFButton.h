//
// Created by Chen Tong on 2026/1/24.
//

//
// Created by Leaf Engine Team.
// UI Component - Button
//

#ifndef LEAF_LFBUTTON_H
#define LEAF_LFBUTTON_H

#include "LFDef.h"
#include "LFBox.h"
#include "animation/LFAnimator.h"
#include "LFText.h"
#include "LFGlobalAnimationManager.h"

enum class LFButtonState {
    Normal,
    Pressed,
    Disabled
};

enum class LFClickEffect {
    None,
    Scale,      // 缩放回弹 (iOS 风格)
    Ripple      // 涟漪 (暂未实现，预留)
};

/**
 * 现代化的按钮组件
 * 特性：
 * 1. 物理弹簧缩放效果 (Spring Scale)
 * 2. 状态背景色平滑过渡
 * 3. 智能触摸追踪 (移出取消/移回恢复)
 */
class LFButton : public LFBox {
public:
    using Ptr = std::shared_ptr<LFButton>;
    using ClickCallback = std::function<void(LFButton* sender)>;

    static Ptr create(const std::string& text = "", ClickCallback onClick = nullptr);

    LFButton();

    ~LFButton() override;

    // =========================
    // 基础配置
    // =========================

    // 设置是否可用 (Disabled 状态)
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_isEnabled; }

    // 设置点击回调
    void setOnClick(ClickCallback callback);

    // 设置点击效果类型
    void setClickEffect(LFClickEffect effect) { m_clickEffect = effect; }

    // =========================
    // 样式配置
    // =========================

    // 为不同状态设置背景色
    void setBackgroundColor(LFButtonState state, uint32_t color);

    // 快捷设置文字 (自动创建/更新内部的 LFText 子节点)
    void setText(const std::string& text);
    void setTextColor(uint32_t color);
    void setFontSize(float size);

    // 获取内部的 Text 节点以便进行更细致的样式调整
    std::shared_ptr<LFText> getTextNode() const { return m_textNode; }

protected:
    // 覆盖父类方法，确保动画更新时能够正确重绘
    // (LFNode 默认逻辑已经支持，不需要额外覆盖 render)

private:
    // 初始化事件监听
    void initEvents();

    // 状态更新逻辑
    void updateState(LFButtonState newState, bool animate = true);

    // 动画执行方法
    void runScaleAnimation(float targetScale);
    void runColorAnimation(uint32_t targetColor);

    // --- 属性 ---
    bool m_isEnabled = true;
    LFButtonState m_state = LFButtonState::Normal;
    LFClickEffect m_clickEffect = LFClickEffect::Scale;

    // 状态样式表
    std::map<LFButtonState, uint32_t> m_stateColors;

    // 内部组件
    std::shared_ptr<LFText> m_textNode = nullptr;
    ClickCallback m_onClick = nullptr;

    // 交互状态追踪
    bool m_isTouching = false;      // 物理上是否按着
    bool m_isInside = false;        // 手指是否在范围内

    // 为了简单，我们持有 shared_ptr，并在析构或 stop 时从引擎移除
    // 动画实例
    // 使用 shared_ptr 强引用，确保组件存活期间动画对象一直有效
    // 注意：在 Callback 中必须使用 weak_ptr 捕获 this，防止循环引用
    std::shared_ptr<LFValueAnimator<float>> m_scaleAnimator;
    std::shared_ptr<LFValueAnimator<uint32_t>> m_colorAnimator;
};

#endif //LEAF_LFBUTTON_H
