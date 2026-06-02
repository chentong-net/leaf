//
// Created by Chen Tong on 2026/6/2.
// UI Plugin - Checkbox Implementation
//

#include "LFCheckbox.h"

#include <algorithm>
#include <utility>

namespace {

constexpr float kMinBoxSize = 14.0f;
constexpr float kMinCornerRadius = 0.0f;
constexpr float kMinBorderWidth = 0.0f;
constexpr float kMinFontSize = 8.0f;

float clampNonNegative(float value) {
    return std::max(0.0f, value);
}

}

LFCheckbox::Ptr LFCheckbox::create(const std::string& text, bool checked) {
    auto checkbox = std::make_shared<LFCheckbox>();
    checkbox->setText(text);
    checkbox->setChecked(checked, false);
    return checkbox;
}

LFCheckbox::LFCheckbox() {
    setOrientation(LFOrientation::Horizontal);
    setAlignItems(YGAlignCenter);
    setSpacing(m_labelSpacing);
    wrapContentWidth();
    wrapContentHeight();
    setTouchEnabled(true);
    setBackgroundColor(0x00000000);

    initLayout();
    initInteractions();
    updateVisualState();
}

void LFCheckbox::setChecked(bool checked, bool notify) {
    if (m_checked == checked) return;

    m_checked = checked;
    updateVisualState();

    if (notify && m_onCheckedChanged) {
        m_onCheckedChanged(m_checked);
    }
}

void LFCheckbox::toggle(bool notify) {
    setChecked(!m_checked, notify);
}

void LFCheckbox::setText(const std::string& text) {
    if (m_text == text) return;

    m_text = text;
    updateVisualState();
}

void LFCheckbox::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;

    m_enabled = enabled;
    updateVisualState();
}

void LFCheckbox::setOnCheckedChanged(CheckedChangedCallback callback) {
    m_onCheckedChanged = std::move(callback);
}

void LFCheckbox::setBoxSize(float size) {
    size = std::max(kMinBoxSize, size);
    if (m_boxSize == size) return;

    m_boxSize = size;
    updateIndicatorGeometry();
    updateVisualState();
}

void LFCheckbox::setCornerRadius(float radius) {
    radius = std::max(kMinCornerRadius, radius);
    if (m_cornerRadius == radius) return;

    m_cornerRadius = radius;
    updateIndicatorGeometry();
}

void LFCheckbox::setBorderWidth(float width) {
    width = std::max(kMinBorderWidth, width);
    if (m_borderWidth == width) return;

    m_borderWidth = width;
    updateVisualState();
}

void LFCheckbox::setFontSize(float size) {
    size = std::max(kMinFontSize, size);
    if (m_fontSize == size) return;

    m_fontSize = size;
    updateVisualState();
}

void LFCheckbox::setLabelSpacing(float spacing) {
    spacing = clampNonNegative(spacing);
    if (m_labelSpacing == spacing) return;

    m_labelSpacing = spacing;
    setSpacing(m_labelSpacing);
}

void LFCheckbox::setTextColor(uint32_t uncheckedColor, uint32_t checkedColor) {
    if (m_uncheckedTextColor == uncheckedColor &&
        m_checkedTextColor == checkedColor) {
        return;
    }

    m_uncheckedTextColor = uncheckedColor;
    m_checkedTextColor = checkedColor;
    updateVisualState();
}

void LFCheckbox::setIndicatorColor(uint32_t uncheckedColor, uint32_t checkedColor) {
    if (m_uncheckedIndicatorColor == uncheckedColor &&
        m_checkedIndicatorColor == checkedColor) {
        return;
    }

    m_uncheckedIndicatorColor = uncheckedColor;
    m_checkedIndicatorColor = checkedColor;
    updateVisualState();
}

void LFCheckbox::setBorderColor(uint32_t uncheckedColor, uint32_t checkedColor) {
    if (m_uncheckedBorderColor == uncheckedColor &&
        m_checkedBorderColor == checkedColor) {
        return;
    }

    m_uncheckedBorderColor = uncheckedColor;
    m_checkedBorderColor = checkedColor;
    updateVisualState();
}

void LFCheckbox::setCheckmarkColor(uint32_t color) {
    if (m_checkmarkColor == color) return;

    m_checkmarkColor = color;
    updateVisualState();
}

void LFCheckbox::setDisabledOpacity(float opacity) {
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (m_disabledOpacity == opacity) return;

    m_disabledOpacity = opacity;
    updateVisualState();
}

void LFCheckbox::initLayout() {
    m_indicator = LFBox::create();
    m_indicator->setTouchEnabled(false);
    addChild(m_indicator);

    m_checkmark = std::make_shared<LFText>();
    m_checkmark->setText("√");
    m_checkmark->setTouchEnabled(false);
    m_checkmark->setTextHAlign(LFTextHAlign::Center);
    m_checkmark->setTextVAlign(LFTextVAlign::Center);
    m_indicator->addChild(m_checkmark, LFBoxAlign::Center);

    m_label = std::make_shared<LFText>();
    m_label->setTouchEnabled(false);
    m_label->setLineHeight(1.25f);
    m_label->setTextVAlign(LFTextVAlign::Center);
    addChild(m_label);

    updateIndicatorGeometry();
}

void LFCheckbox::initInteractions() {
    setOnTap([this](const LFPoint&) {
        if (!m_enabled) return;
        toggle(true);
    });
}

void LFCheckbox::updateVisualState() {
    updateIndicatorGeometry();

    setSpacing(m_labelSpacing);
    setOpacity(m_enabled ? 1.0f : m_disabledOpacity);

    if (m_indicator) {
        m_indicator->setBackgroundColor(m_checked ? m_checkedIndicatorColor : m_uncheckedIndicatorColor);
        m_indicator->setBorder(m_borderWidth, m_checked ? m_checkedBorderColor : m_uncheckedBorderColor);
    }

    if (m_checkmark) {
        m_checkmark->setTextColor(m_checkmarkColor);
        m_checkmark->setDisplay(m_checked ? YGDisplayFlex : YGDisplayNone);
    }

    if (m_label) {
        m_label->setText(m_text);
        m_label->setFontSize(m_fontSize);
        m_label->setTextColor(m_checked ? m_checkedTextColor : m_uncheckedTextColor);
        m_label->setDisplay(m_text.empty() ? YGDisplayNone : YGDisplayFlex);
    }
}

void LFCheckbox::updateIndicatorGeometry() {
    if (!m_indicator) return;

    m_indicator->setWidth(m_boxSize);
    m_indicator->setHeight(m_boxSize);
    m_indicator->setBorderRadius(std::min(m_cornerRadius, m_boxSize * 0.5f));

    if (m_checkmark) {
        m_checkmark->setFontSize(std::max(kMinFontSize, m_boxSize * 0.7f));
    }
}
