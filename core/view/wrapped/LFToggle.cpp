//
// Created by Chen Tong on 2026/5/30.
// UI Component - Toggle Implementation
//

#include "LFToggle.h"

#include <algorithm>
#include <cmath>

#include "animation/LFEvaluator.h"
#include "LFGlobalAnimationManager.h"
#include "event/LFEvent.h"

namespace {

constexpr float kEpsilon = 0.01f;

bool almostEqual(float a, float b) {
    return std::fabs(a - b) <= kEpsilon;
}

float clampNonNegative(float value) {
    return std::max(0.0f, value);
}

}

LFToggle::Ptr LFToggle::create(bool checked) {
    auto toggle = std::make_shared<LFToggle>();
    toggle->setChecked(checked, false, false);
    return toggle;
}

LFToggle::LFToggle() {
    initLayout();
    initInteractions();
    applyVisualProgress(0.0f);
}

LFToggle::~LFToggle() {
    stopProgressAnimation();
}

void LFToggle::setChecked(bool checked, bool notify, bool animate) {
    if (m_checked == checked) return;

    m_checked = checked;
    updateVisualState(animate);

    if (notify && m_onCheckedChanged) {
        m_onCheckedChanged(m_checked);
    }
}

void LFToggle::toggle(bool notify, bool animate) {
    setChecked(!m_checked, notify, animate);
}

void LFToggle::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;

    m_enabled = enabled;
    if (!m_enabled && hasFocus()) {
        clearFocus();
    }
    applyVisualProgress(m_visualProgress);
}

void LFToggle::setOnCheckedChanged(CheckedChangedCallback callback) {
    m_onCheckedChanged = std::move(callback);
}

void LFToggle::setTrackSize(float width, float height) {
    // TODO: 宽高设置了最小值，未来考虑取消
    width = std::max(28.0f, width);
    height = std::max(18.0f, height);

    if (almostEqual(m_trackWidth, width) && almostEqual(m_trackHeight, height)) {
        return;
    }

    m_trackWidth = width;
    m_trackHeight = height;

    setWidth(width);
    setHeight(height);

    syncResolvedGeometry();
    applyVisualProgress(m_visualProgress);
}

void LFToggle::setThumbPadding(float padding) {
    padding = clampNonNegative(padding);
    if (almostEqual(m_thumbPadding, padding)) return;

    m_thumbPadding = padding;
    syncResolvedGeometry();
    applyVisualProgress(m_visualProgress);
}

void LFToggle::setTrackColor(uint32_t uncheckedColor, uint32_t checkedColor) {
    if (m_trackUncheckedColor == uncheckedColor &&
        m_trackCheckedColor == checkedColor) {
        return;
    }

    m_trackUncheckedColor = uncheckedColor;
    m_trackCheckedColor = checkedColor;
    applyVisualProgress(m_visualProgress);
}

void LFToggle::setThumbColor(uint32_t uncheckedColor, uint32_t checkedColor) {
    if (m_thumbUncheckedColor == uncheckedColor &&
        m_thumbCheckedColor == checkedColor) {
        return;
    }

    m_thumbUncheckedColor = uncheckedColor;
    m_thumbCheckedColor = checkedColor;
    applyVisualProgress(m_visualProgress);
}

void LFToggle::setBorderColor(uint32_t uncheckedColor, uint32_t checkedColor) {
    if (m_borderUncheckedColor == uncheckedColor &&
        m_borderCheckedColor == checkedColor) {
        return;
    }

    m_borderUncheckedColor = uncheckedColor;
    m_borderCheckedColor = checkedColor;
    applyVisualProgress(m_visualProgress);
}

void LFToggle::setBorderWidth(float width) {
    width = clampNonNegative(width);
    if (almostEqual(m_borderWidth, width)) return;

    m_borderWidth = width;
    applyVisualProgress(m_visualProgress);
}

void LFToggle::setDisabledOpacity(float opacity) {
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (almostEqual(m_disabledOpacity, opacity)) return;

    m_disabledOpacity = opacity;
    applyVisualProgress(m_visualProgress);
}

void LFToggle::onAfterCalculateLayout() {
    LFBox::onAfterCalculateLayout();
    syncResolvedGeometry();
    applyVisualProgress(m_visualProgress);
}

void LFToggle::initLayout() {
    setWidth(m_trackWidth);
    setHeight(m_trackHeight);
    setFocusable(true);
    setTouchEnabled(true);

    m_thumb = LFBox::create();
    m_thumb->setTouchEnabled(false);
    m_thumb->setShadow(0.0f, 2.0f, 6.0f, 0.0f, 0x26000000);
    addChild(m_thumb, LFBoxAlign::CenterLeft);

    syncResolvedGeometry();
}

void LFToggle::initInteractions() {
    setOnTouchDown([this](const LFTouchEvent&) {
        if (!m_enabled) return;
        requestFocus();
    });

    setOnTap([this](const LFPoint&) {
        if (!m_enabled) return;
        toggle(true, true);
    });

    setOnKeyDown([this](LFKeyEvent& event) {
        if (!m_enabled) return;

        if (event.keyCode == LFKeyCode::Enter) {
            toggle(true, true);
            event.stopPropagation();
        } else if (event.keyCode == LFKeyCode::Left) {
            setChecked(false, true, true);
            event.stopPropagation();
        } else if (event.keyCode == LFKeyCode::Right) {
            setChecked(true, true, true);
            event.stopPropagation();
        }
    });
}

void LFToggle::updateVisualState(bool animate) {
    float targetProgress = m_checked ? 1.0f : 0.0f;

    if (animate) {
        startProgressAnimation(targetProgress);
    } else {
        stopProgressAnimation();
        m_visualProgress = targetProgress;
        applyVisualProgress(m_visualProgress);
    }
}

void LFToggle::applyVisualProgress(float progress) {
    progress = std::clamp(progress, 0.0f, 1.0f);
    m_visualProgress = progress;

    syncResolvedGeometry();

    LFColorEvaluator colorEvaluator;
    uint32_t trackColor = colorEvaluator.evaluate(progress, m_trackUncheckedColor, m_trackCheckedColor);
    uint32_t thumbColor = colorEvaluator.evaluate(progress, m_thumbUncheckedColor, m_thumbCheckedColor);
    uint32_t borderColor = colorEvaluator.evaluate(progress, m_borderUncheckedColor, m_borderCheckedColor);

    LFNode::setBackgroundColor(trackColor);
    setBorder(m_borderWidth, borderColor);
    setOpacity(m_enabled ? 1.0f : m_disabledOpacity);

    if (!m_thumb) return;

    m_thumb->setBackgroundColor(thumbColor);

    float travel = std::max(0.0f, m_resolvedTrackWidth - m_resolvedThumbDiameter - (m_thumbPadding * 2.0f));
    m_thumb->setTranslate(m_thumbPadding + travel * progress, 0.0f);
}

void LFToggle::syncResolvedGeometry() {
    float trackWidth = resolveTrackWidth();
    float trackHeight = resolveTrackHeight();
    float thumbDiameter = std::max(0.0f, trackHeight - (m_thumbPadding * 2.0f));
    float maxThumbDiameter = std::max(0.0f, trackWidth - (m_thumbPadding * 2.0f));
    thumbDiameter = std::min(thumbDiameter, maxThumbDiameter);

    if (!almostEqual(m_resolvedTrackWidth, trackWidth) ||
        !almostEqual(m_resolvedTrackHeight, trackHeight)) {
        m_resolvedTrackWidth = trackWidth;
        m_resolvedTrackHeight = trackHeight;
        setBorderRadius(trackHeight * 0.5f);
    }

    if (m_thumb &&
        !almostEqual(m_resolvedThumbDiameter, thumbDiameter)) {
        m_resolvedThumbDiameter = thumbDiameter;
        m_thumb->setWidth(thumbDiameter);
        m_thumb->setHeight(thumbDiameter);
        m_thumb->setBorderRadius(thumbDiameter * 0.5f);
    }
}

void LFToggle::startProgressAnimation(float targetProgress) {
    targetProgress = std::clamp(targetProgress, 0.0f, 1.0f);
    if (almostEqual(m_visualProgress, targetProgress)) {
        m_visualProgress = targetProgress;
        applyVisualProgress(m_visualProgress);
        return;
    }

    stopProgressAnimation();

    m_progressAnimator = LFValueAnimator<float>::of(m_visualProgress, targetProgress);
    m_progressAnimator->setDuration(m_animationDuration);
    m_progressAnimator->setEasing(LFEasingType::QuadOut);

    std::weak_ptr<LFNode> weakSelf = shared_from_this();
    m_progressAnimator->addUpdateListener([weakSelf](const float& value) {
        auto self = std::static_pointer_cast<LFToggle>(weakSelf.lock());
        if (!self) return;
        self->applyVisualProgress(value);
    });

    m_progressAnimator->start();
    LFGlobalAnimationManager::getInstance().addAnimator(m_progressAnimator);
}

void LFToggle::stopProgressAnimation() {
    if (m_progressAnimator && m_progressAnimator->isRunning()) {
        m_progressAnimator->stop();
    }
}

float LFToggle::resolveTrackWidth() const {
    float width = getLayoutWidth();
    return width > 0.0f ? width : m_trackWidth;
}

float LFToggle::resolveTrackHeight() const {
    float height = getLayoutHeight();
    return height > 0.0f ? height : m_trackHeight;
}
