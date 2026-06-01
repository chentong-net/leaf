//
// Created by Chen Tong on 2026/5/31.
// UI Component - Slider
//

#ifndef LEAF_LFSLIDER_H
#define LEAF_LFSLIDER_H

#include "event/LFEvent.h"
#include "view/layout/LFBox.h"

/**
 * 横向滑块组件
 *
 * 结构：
 * - Root: 可交互区域
 * - Track: 背景轨道
 * - Progress: 已选区
 * - Thumb: 滑块
 */
class LFSlider : public LFBox {
public:
    using Ptr = std::shared_ptr<LFSlider>;
    using ValueChangedCallback = std::function<void(float value)>;
    using DragCallback = std::function<void(float value)>;

    static Ptr create(float minValue = 0.0f, float maxValue = 1.0f, float value = 0.0f);

    LFSlider();
    ~LFSlider() override = default;

    void setRange(float minValue, float maxValue);
    float getMinValue() const { return m_minValue; }
    float getMaxValue() const { return m_maxValue; }

    void setValue(float value, bool notify = true);
    float getValue() const { return m_value; }
    float getNormalizedValue() const;

    void setStep(float step);
    float getStep() const { return m_step; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void setTrackThickness(float thickness);
    float getTrackThickness() const { return m_trackThickness; }

    void setThumbDiameter(float diameter);
    float getThumbDiameter() const { return m_thumbDiameter; }

    void setTrackColor(uint32_t color);
    void setProgressColor(uint32_t color);
    void setThumbColor(uint32_t color);
    void setDisabledOpacity(float opacity);

    void setOnValueChanged(ValueChangedCallback callback);
    void setOnDragBegin(DragCallback callback);
    void setOnDragEnd(DragCallback callback);

    LFBox::Ptr getTrackNode() const { return m_track; }
    LFBox::Ptr getProgressNode() const { return m_progress; }
    LFBox::Ptr getThumbNode() const { return m_thumb; }

protected:
    void onBeforeCalculateLayout(float ownerWidth, float ownerHeight) override;
    void onAfterCalculateLayout() override;

private:
    void initLayout();
    void initInteractions();
    void syncVisualState();
    void updateTrackStyle();
    void updateThumbStyle();
    void updateRootSize();
    void updateStateFromLocalX(float localX, bool notify, bool snapToStep);
    void setRawValue(float value, bool notify);
    float valueFromLocalX(float localX) const;
    float resolveTrackWidth() const;
    float clampValue(float value) const;
    float quantizeValue(float value) const;
    float clampNormalized(float normalized) const;
    LFPoint toLocalPoint(const LFTouchPoint& touch) const;
    float getRootAbsoluteX() const;
    float getRootAbsoluteY() const;

    static bool almostEqual(float a, float b);

    LFBox::Ptr m_track;
    LFBox::Ptr m_progress;
    LFBox::Ptr m_thumb;
    ValueChangedCallback m_onValueChanged;
    DragCallback m_onDragBegin;
    DragCallback m_onDragEnd;

    float m_minValue = 0.0f;
    float m_maxValue = 1.0f;
    float m_value = 0.0f;
    float m_step = 0.0f;
    bool m_enabled = true;
    bool m_isDragging = false;

    float m_trackThickness = 6.0f;
    float m_thumbDiameter = 20.0f;
    float m_disabledOpacity = 0.55f;

    uint32_t m_trackColor = 0xFFE5E7EB;
    uint32_t m_progressColor = 0xFF3567A3;
    uint32_t m_thumbColor = 0xFFFFFFFF;

    float m_fallbackWidth = 240.0f;

    float m_resolvedTrackWidth = -1.0f;
    float m_resolvedProgressWidth = -1.0f;
    float m_resolvedThumbX = -1.0f;
    float m_resolvedProgressRadius = -1.0f;
};

#endif // LEAF_LFSLIDER_H
