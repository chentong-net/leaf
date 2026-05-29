//
// Created by Chen Tong on 2026/5/30.
// UI Component - Toggle
//

#ifndef LEAF_LFTOGGLE_H
#define LEAF_LFTOGGLE_H

#include "animation/LFAnimator.h"
#include "view/layout/LFBox.h"

/**
 * 开关组件
 *
 * 结构：
 * - Root: 轨道
 * - Thumb: 滑块
 *
 * 特性：
 * 1. 点击切换选中状态
 * 2. 轨道颜色与滑块位置动画
 * 3. 支持禁用态
 * 4. 支持基础样式配置
 */
class LFToggle : public LFBox {
public:
    using Ptr = std::shared_ptr<LFToggle>;
    using CheckedChangedCallback = std::function<void(bool checked)>;

    static Ptr create(bool checked = false);

    LFToggle();
    ~LFToggle() override;

    void setChecked(bool checked, bool notify = true, bool animate = true);
    bool isChecked() const { return m_checked; }
    void toggle(bool notify = true, bool animate = true);

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void setOnCheckedChanged(CheckedChangedCallback callback);

    void setTrackSize(float width, float height);
    float getTrackWidth() const { return m_trackWidth; }
    float getTrackHeight() const { return m_trackHeight; }

    void setThumbPadding(float padding);
    float getThumbPadding() const { return m_thumbPadding; }

    void setTrackColor(uint32_t uncheckedColor, uint32_t checkedColor);
    void setThumbColor(uint32_t uncheckedColor, uint32_t checkedColor);
    void setBorderColor(uint32_t uncheckedColor, uint32_t checkedColor);
    void setBorderWidth(float width);
    void setDisabledOpacity(float opacity);

    LFBox::Ptr getThumbNode() const { return m_thumb; }

protected:
    void onAfterCalculateLayout() override;

private:
    void initLayout();
    void initInteractions();
    void updateVisualState(bool animate);
    void applyVisualProgress(float progress);
    void syncResolvedGeometry();
    void startProgressAnimation(float targetProgress);
    void stopProgressAnimation();
    float resolveTrackWidth() const;
    float resolveTrackHeight() const;

    LFBox::Ptr m_thumb;
    CheckedChangedCallback m_onCheckedChanged;

    bool m_checked = false;
    bool m_enabled = true;

    float m_trackWidth = 52.0f;
    float m_trackHeight = 32.0f;
    float m_thumbPadding = 3.0f;
    float m_borderWidth = 1.0f;
    float m_disabledOpacity = 0.55f;
    float m_animationDuration = 0.18f;
    float m_visualProgress = 0.0f;

    float m_resolvedTrackWidth = -1.0f;
    float m_resolvedTrackHeight = -1.0f;
    float m_resolvedThumbDiameter = -1.0f;

    uint32_t m_trackUncheckedColor = 0xFFE5E7EB;
    uint32_t m_trackCheckedColor = 0xFF34C759;
    uint32_t m_thumbUncheckedColor = 0xFFFFFFFF;
    uint32_t m_thumbCheckedColor = 0xFFFFFFFF;
    uint32_t m_borderUncheckedColor = 0xFFD0D7DE;
    uint32_t m_borderCheckedColor = 0xFF2FB053;

    std::shared_ptr<LFValueAnimator<float>> m_progressAnimator;
};

#endif // LEAF_LFTOGGLE_H
