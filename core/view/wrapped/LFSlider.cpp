//
// Created by Chen Tong on 2026/5/31.
// UI Component - Slider Implementation
//

#include "LFSlider.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

constexpr float kMinTrackThickness = 2.0f;
constexpr float kMinThumbDiameter = 8.0f;
constexpr float kMinWidth = 24.0f;

}

LFSlider::Ptr LFSlider::create(float minValue, float maxValue, float value) {
    auto slider = std::make_shared<LFSlider>();
    slider->setRange(minValue, maxValue);
    slider->setValue(value, false);
    return slider;
}

LFSlider::LFSlider() {
    matchParentWidth();
    setHeight(std::max(m_trackThickness, m_thumbDiameter));
    setTouchEnabled(true);
    setBackgroundColor(0x00000000);

    initLayout();
    initInteractions();
    syncVisualState();
}

void LFSlider::setRange(float minValue, float maxValue) {
    if (almostEqual(minValue, maxValue)) {
        maxValue = minValue + 1.0f;
    } else if (maxValue < minValue) {
        std::swap(minValue, maxValue);
    }

    if (almostEqual(m_minValue, minValue) && almostEqual(m_maxValue, maxValue)) {
        return;
    }

    m_minValue = minValue;
    m_maxValue = maxValue;
    m_value = clampValue(quantizeValue(m_value));
    syncVisualState();
}

void LFSlider::setValue(float value, bool notify) {
    float nextValue = clampValue(quantizeValue(value));
    if (almostEqual(m_value, nextValue)) {
        return;
    }

    m_value = nextValue;
    syncVisualState();

    if (notify && m_onValueChanged) {
        m_onValueChanged(m_value);
    }
}

float LFSlider::getNormalizedValue() const {
    float range = m_maxValue - m_minValue;
    if (almostEqual(range, 0.0f)) {
        return 0.0f;
    }
    return clampNormalized((m_value - m_minValue) / range);
}

void LFSlider::setStep(float step) {
    step = std::max(0.0f, step);
    if (almostEqual(m_step, step)) {
        return;
    }

    m_step = step;
    setValue(m_value, false);
}

void LFSlider::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;

    m_enabled = enabled;
    if (!m_enabled) {
        m_isDragging = false;
    }

    setTouchEnabled(enabled);
    setOpacity(m_enabled ? 1.0f : m_disabledOpacity);
}

void LFSlider::setTrackThickness(float thickness) {
    thickness = std::max(kMinTrackThickness, thickness);
    if (almostEqual(m_trackThickness, thickness)) return;

    m_trackThickness = thickness;
    updateRootSize();
    updateTrackStyle();
}

void LFSlider::setThumbDiameter(float diameter) {
    diameter = std::max(kMinThumbDiameter, diameter);
    if (almostEqual(m_thumbDiameter, diameter)) return;

    m_thumbDiameter = diameter;
    updateRootSize();
    updateThumbStyle();
    syncVisualState();
}

void LFSlider::setTrackColor(uint32_t color) {
    if (m_trackColor == color) return;
    m_trackColor = color;
    updateTrackStyle();
}

void LFSlider::setProgressColor(uint32_t color) {
    if (m_progressColor == color) return;
    m_progressColor = color;
    updateTrackStyle();
}

void LFSlider::setThumbColor(uint32_t color) {
    if (m_thumbColor == color) return;
    m_thumbColor = color;
    updateThumbStyle();
}

void LFSlider::setDisabledOpacity(float opacity) {
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (almostEqual(m_disabledOpacity, opacity)) return;

    m_disabledOpacity = opacity;
    if (!m_enabled) {
        setOpacity(m_disabledOpacity);
    }
}

void LFSlider::setOnValueChanged(ValueChangedCallback callback) {
    m_onValueChanged = std::move(callback);
}

void LFSlider::setOnDragBegin(DragCallback callback) {
    m_onDragBegin = std::move(callback);
}

void LFSlider::setOnDragEnd(DragCallback callback) {
    m_onDragEnd = std::move(callback);
}

void LFSlider::onBeforeCalculateLayout(float ownerWidth, float ownerHeight) {
    syncVisualState();
    LFBox::onBeforeCalculateLayout(ownerWidth, ownerHeight);
}

void LFSlider::onAfterCalculateLayout() {
    LFBox::onAfterCalculateLayout();
    syncVisualState();
}

void LFSlider::initLayout() {
    m_track = LFBox::create();
    m_track->matchParentWidth();
    m_track->setHeight(m_trackThickness);
    m_track->setBackgroundColor(m_trackColor);
    m_track->setBorderRadius(m_trackThickness * 0.5f);
    m_track->setTouchEnabled(false);
    LFBox::addChild(m_track, LFBoxAlign::CenterLeft);

    m_progress = LFBox::create();
    m_progress->setHeight(m_trackThickness);
    m_progress->setBackgroundColor(m_progressColor);
    m_progress->setBorderRadius(m_trackThickness * 0.5f);
    m_progress->setTouchEnabled(false);
    m_track->addChild(m_progress, LFBoxAlign::TopLeft);

    m_thumb = LFBox::create();
    m_thumb->setWidth(m_thumbDiameter);
    m_thumb->setHeight(m_thumbDiameter);
    m_thumb->setBackgroundColor(m_thumbColor);
    m_thumb->setBorderRadius(m_thumbDiameter * 0.5f);
    m_thumb->setShadow(0.0f, 2.0f, 8.0f, 0.0f, 0x24000000);
    m_thumb->setTouchEnabled(false);
    m_track->addChild(m_thumb, LFBoxAlign::CenterLeft);

    setOpacity(1.0f);
}

void LFSlider::initInteractions() {
    setOnTouchDown([this](const LFTouchEvent& event) {
        if (!m_enabled) return;
        m_isDragging = true;

        if (m_onDragBegin) {
            m_onDragBegin(m_value);
        }

        if (const auto* touch = event.getPrimaryTouch()) {
            updateStateFromLocalX(toLocalPoint(*touch).x, true, false);
        }
    });

    setOnTouchMove([this](const LFTouchEvent& event) {
        if (!m_enabled || !m_isDragging) return;

        if (const auto* touch = event.getPrimaryTouch()) {
            updateStateFromLocalX(toLocalPoint(*touch).x, true, false);
        }
    });

    setOnTouchUp([this](const LFTouchEvent& event) {
        if (!m_enabled || !m_isDragging) return;
        m_isDragging = false;

        if (const auto* touch = event.getPrimaryTouch()) {
            updateStateFromLocalX(toLocalPoint(*touch).x, false, false);
        }
        setValue(m_value, true);
        if (m_onDragEnd) {
            m_onDragEnd(m_value);
        }
    });

    setOnTouchCancel([this](const LFTouchEvent&) {
        m_isDragging = false;
        setValue(m_value, false);
    });
}

void LFSlider::syncVisualState() {
    if (!m_track || !m_progress || !m_thumb) return;

    float trackWidth = resolveTrackWidth();
    trackWidth = std::max(kMinWidth, trackWidth);

    float normalized = getNormalizedValue();
    float thumbDiameter = std::max(kMinThumbDiameter, m_thumbDiameter);
    float travel = std::max(0.0f, trackWidth - thumbDiameter);
    float thumbX = travel * normalized;
    float progressWidth = std::min(trackWidth, thumbX + thumbDiameter * 0.5f);
    float progressRadius = std::min(m_trackThickness, progressWidth) * 0.5f;

    if (!almostEqual(m_resolvedTrackWidth, trackWidth)) {
        m_resolvedTrackWidth = trackWidth;
    }

    if (!almostEqual(m_resolvedProgressWidth, progressWidth)) {
        m_resolvedProgressWidth = progressWidth;
        m_progress->setWidth(progressWidth);
    }

    if (!almostEqual(m_resolvedThumbX, thumbX)) {
        m_resolvedThumbX = thumbX;
        m_thumb->setTranslate(thumbX, 0.0f);
    }

    if (!almostEqual(m_resolvedProgressRadius, progressRadius)) {
        m_resolvedProgressRadius = progressRadius;
        m_progress->setBorderRadius(progressRadius);
    }

    updateTrackStyle();
    updateThumbStyle();

    setOpacity(m_enabled ? 1.0f : m_disabledOpacity);
}

void LFSlider::updateTrackStyle() {
    if (!m_track || !m_progress) return;

    if (m_track->getBackgroundColor() != m_trackColor) {
        m_track->setBackgroundColor(m_trackColor);
    }
    if (m_progress->getBackgroundColor() != m_progressColor) {
        m_progress->setBackgroundColor(m_progressColor);
    }

    if (!almostEqual(m_track->getLayoutHeight(), m_trackThickness)) {
        m_track->setHeight(m_trackThickness);
    }
    if (!almostEqual(m_progress->getLayoutHeight(), m_trackThickness)) {
        m_progress->setHeight(m_trackThickness);
    }
    if (!almostEqual(m_track->getRadius(), m_trackThickness * 0.5f)) {
        m_track->setBorderRadius(m_trackThickness * 0.5f);
    }
}

void LFSlider::updateThumbStyle() {
    if (!m_thumb) return;

    if (m_thumb->getBackgroundColor() != m_thumbColor) {
        m_thumb->setBackgroundColor(m_thumbColor);
    }

    if (!almostEqual(m_thumb->getLayoutWidth(), m_thumbDiameter)) {
        m_thumb->setWidth(m_thumbDiameter);
    }
    if (!almostEqual(m_thumb->getLayoutHeight(), m_thumbDiameter)) {
        m_thumb->setHeight(m_thumbDiameter);
    }
    if (!almostEqual(m_thumb->getRadius(), m_thumbDiameter * 0.5f)) {
        m_thumb->setBorderRadius(m_thumbDiameter * 0.5f);
    }
}

void LFSlider::updateRootSize() {
    setHeight(std::max(m_trackThickness, m_thumbDiameter));
}

void LFSlider::updateStateFromLocalX(float localX, bool notify, bool snapToStep) {
    float nextValue = valueFromLocalX(localX);
    if (snapToStep) {
        setValue(nextValue, notify);
    } else {
        setRawValue(nextValue, notify);
    }
}

void LFSlider::setRawValue(float value, bool notify) {
    float nextValue = clampValue(value);
    if (almostEqual(m_value, nextValue)) {
        return;
    }

    m_value = nextValue;
    syncVisualState();

    if (notify && m_onValueChanged) {
        m_onValueChanged(m_value);
    }
}

float LFSlider::valueFromLocalX(float localX) const {
    float trackWidth = std::max(kMinWidth, resolveTrackWidth());
    float thumbDiameter = std::max(kMinThumbDiameter, m_thumbDiameter);
    float travel = std::max(0.0f, trackWidth - thumbDiameter);
    float normalized = travel <= 0.0f ? 0.0f : (localX - thumbDiameter * 0.5f) / travel;
    normalized = clampNormalized(normalized);
    float range = m_maxValue - m_minValue;
    return m_minValue + range * normalized;
}

float LFSlider::resolveTrackWidth() const {
    if (m_track) {
        float trackWidth = m_track->getLayoutWidth();
        if (trackWidth > 0.0f) {
            return trackWidth;
        }
    }

    float layoutWidth = getLayoutWidth();
    if (layoutWidth > 0.0f) {
        return layoutWidth;
    }
    return m_fallbackWidth;
}

float LFSlider::clampValue(float value) const {
    return std::clamp(value, m_minValue, m_maxValue);
}

float LFSlider::quantizeValue(float value) const {
    value = clampValue(value);
    if (m_step <= 0.0f) {
        return value;
    }

    float steps = std::round((value - m_minValue) / m_step);
    return clampValue(m_minValue + steps * m_step);
}

float LFSlider::clampNormalized(float normalized) const {
    return std::clamp(normalized, 0.0f, 1.0f);
}

LFPoint LFSlider::toLocalPoint(const LFTouchPoint& touch) const {
    return LFPoint(touch.x - getRootAbsoluteX(), touch.y - getRootAbsoluteY());
}

float LFSlider::getRootAbsoluteX() const {
    // TODO: 没有处理scale/rotate
    float x = 0.0f;
    const LFNode* current = this;
    while (current) {
        x += current->getLayoutX();
        const auto& transform = current->getTransform();
        x += transform.translateX;
        x += current->getLayoutWidth() * (transform.translatePercentX / 100.0f);
        current = current->getParent();
    }
    return x;
}

float LFSlider::getRootAbsoluteY() const {
    float y = 0.0f;
    const LFNode* current = this;
    while (current) {
        y += current->getLayoutY();
        const auto& transform = current->getTransform();
        y += transform.translateY;
        y += current->getLayoutHeight() * (transform.translatePercentY / 100.0f);
        current = current->getParent();
    }
    return y;
}

bool LFSlider::almostEqual(float a, float b) {
    return std::fabs(a - b) <= 0.001f;
}
