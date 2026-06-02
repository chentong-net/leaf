//
// Created by Chen Tong on 2026/6/2.
// UI Plugin - Radio Group Implementation
//

#include "LFRadioGroup.h"

#include <algorithm>
#include <utility>

namespace {

constexpr float kMinIndicatorSize = 14.0f;
constexpr float kMinInnerDotSize = 4.0f;
constexpr float kMinBorderWidth = 0.0f;
constexpr float kMinFontSize = 8.0f;

float clampNonNegative(float value) {
    return std::max(0.0f, value);
}

}

LFRadioGroup::Ptr LFRadioGroup::create() {
    return std::make_shared<LFRadioGroup>();
}

LFRadioGroup::Ptr LFRadioGroup::create(const std::vector<std::string>& options) {
    auto group = std::make_shared<LFRadioGroup>();
    group->setOptions(options);
    return group;
}

LFRadioGroup::LFRadioGroup() {
    initLayout();
}

void LFRadioGroup::setOptions(const std::vector<std::string>& options) {
    m_options = options;
    if (!isValidIndex(m_selectedIndex)) {
        m_selectedIndex = -1;
    }
    rebuildOptions();
}

void LFRadioGroup::setSelectedIndex(int index, bool notify) {
    if (!isValidIndex(index)) {
        index = -1;
    }
    if (m_selectedIndex == index) return;

    m_selectedIndex = index;
    updateVisualState();

    if (notify && m_onSelectionChanged && isValidIndex(m_selectedIndex)) {
        m_onSelectionChanged(m_selectedIndex, m_options[static_cast<size_t>(m_selectedIndex)]);
    }
}

std::string LFRadioGroup::getSelectedValue() const {
    if (!isValidIndex(m_selectedIndex)) {
        return "";
    }
    return m_options[static_cast<size_t>(m_selectedIndex)];
}

void LFRadioGroup::clearSelection(bool notify) {
    setSelectedIndex(-1, notify);
}

void LFRadioGroup::setOnSelectionChanged(SelectionChangedCallback callback) {
    m_onSelectionChanged = std::move(callback);
}

void LFRadioGroup::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;

    m_enabled = enabled;
    updateVisualState();
}

void LFRadioGroup::setOptionDirection(LFOrientation orientation) {
    if (m_optionDirection == orientation) return;

    m_optionDirection = orientation;
    LFLinear::setOrientation(orientation);
    updateVisualState();
}

void LFRadioGroup::setOptionSpacing(float spacing) {
    spacing = clampNonNegative(spacing);
    if (m_optionSpacing == spacing) return;

    m_optionSpacing = spacing;
    setSpacing(m_optionSpacing);
}

void LFRadioGroup::setItemSpacing(float spacing) {
    spacing = clampNonNegative(spacing);
    if (m_itemSpacing == spacing) return;

    m_itemSpacing = spacing;
    updateVisualState();
}

void LFRadioGroup::setIndicatorSize(float size) {
    size = std::max(kMinIndicatorSize, size);
    if (m_indicatorSize == size) return;

    m_indicatorSize = size;
    if (m_innerDotSize > m_indicatorSize) {
        m_innerDotSize = m_indicatorSize;
    }
    updateVisualState();
}

void LFRadioGroup::setInnerDotSize(float size) {
    size = std::clamp(size, kMinInnerDotSize, m_indicatorSize);
    if (m_innerDotSize == size) return;

    m_innerDotSize = size;
    updateVisualState();
}

void LFRadioGroup::setBorderWidth(float width) {
    width = std::max(kMinBorderWidth, width);
    if (m_borderWidth == width) return;

    m_borderWidth = width;
    updateVisualState();
}

void LFRadioGroup::setFontSize(float size) {
    size = std::max(kMinFontSize, size);
    if (m_fontSize == size) return;

    m_fontSize = size;
    updateVisualState();
}

void LFRadioGroup::setTextColor(uint32_t uncheckedColor, uint32_t checkedColor) {
    if (m_uncheckedTextColor == uncheckedColor &&
        m_checkedTextColor == checkedColor) {
        return;
    }

    m_uncheckedTextColor = uncheckedColor;
    m_checkedTextColor = checkedColor;
    updateVisualState();
}

void LFRadioGroup::setIndicatorColor(uint32_t uncheckedColor, uint32_t checkedColor) {
    if (m_uncheckedIndicatorColor == uncheckedColor &&
        m_checkedIndicatorColor == checkedColor) {
        return;
    }

    m_uncheckedIndicatorColor = uncheckedColor;
    m_checkedIndicatorColor = checkedColor;
    updateVisualState();
}

void LFRadioGroup::setBorderColor(uint32_t uncheckedColor, uint32_t checkedColor) {
    if (m_uncheckedBorderColor == uncheckedColor &&
        m_checkedBorderColor == checkedColor) {
        return;
    }

    m_uncheckedBorderColor = uncheckedColor;
    m_checkedBorderColor = checkedColor;
    updateVisualState();
}

void LFRadioGroup::setItemBackgroundColor(uint32_t uncheckedColor, uint32_t checkedColor) {
    if (m_uncheckedItemBackgroundColor == uncheckedColor &&
        m_checkedItemBackgroundColor == checkedColor) {
        return;
    }

    m_uncheckedItemBackgroundColor = uncheckedColor;
    m_checkedItemBackgroundColor = checkedColor;
    updateVisualState();
}

void LFRadioGroup::setDisabledOpacity(float opacity) {
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (m_disabledOpacity == opacity) return;

    m_disabledOpacity = opacity;
    updateVisualState();
}

void LFRadioGroup::initLayout() {
    setOptionDirection(m_optionDirection);
    setSpacing(m_optionSpacing);
    setTouchEnabled(false);
    setBackgroundColor(0x00000000);
    wrapContentHeight();
}

void LFRadioGroup::rebuildOptions() {
    for (auto& view : m_optionViews) {
        if (view.row) {
            removeChild(view.row);
        }
    }
    m_optionViews.clear();

    for (size_t i = 0; i < m_options.size(); ++i) {
        OptionView view;

        view.row = LFLinear::createHorizontal();
        view.row->wrapContentHeight();
        view.row->setGravity(LFAlignment::Center, LFAlignment::Center);
        view.row->setSpacing(m_itemSpacing);
        view.row->setPadding(YGEdgeLeft, 4.0f);
        view.row->setPadding(YGEdgeRight, 8.0f);
        view.row->setPadding(YGEdgeTop, 6.0f);
        view.row->setPadding(YGEdgeBottom, 6.0f);
        view.row->setBorderRadius(10.0f);
        view.row->setTouchEnabled(true);

        view.indicator = LFBox::create();
        view.indicator->setTouchEnabled(false);
        view.row->addChild(view.indicator);

        view.dot = LFBox::create();
        view.dot->setTouchEnabled(false);
        view.indicator->addChild(view.dot, LFBoxAlign::Center);

        view.label = std::make_shared<LFText>();
        view.label->setTouchEnabled(false);
        view.label->setLineHeight(1.25f);
        view.label->setTextVAlign(LFTextVAlign::Center);
        view.row->addChild(view.label);

        const int index = static_cast<int>(i);
        view.row->setOnTap([this, index](const LFPoint&) {
            if (!m_enabled) return;
            setSelectedIndex(index, true);
        });

        addChild(view.row);
        m_optionViews.push_back(view);
    }

    updateVisualState();
}

void LFRadioGroup::updateVisualState() {
    setSpacing(m_optionSpacing);
    setOpacity(m_enabled ? 1.0f : m_disabledOpacity);

    for (size_t i = 0; i < m_optionViews.size(); ++i) {
        auto& view = m_optionViews[i];
        const bool selected = static_cast<int>(i) == m_selectedIndex;

        updateItemGeometry(view);

        if (view.row) {
            if (m_optionDirection == LFOrientation::Vertical) {
                view.row->matchParentWidth();
            } else {
                view.row->wrapContentWidth();
            }
            view.row->setSpacing(m_itemSpacing);
            view.row->setBackgroundColor(selected ? m_checkedItemBackgroundColor : m_uncheckedItemBackgroundColor);
        }

        if (view.indicator) {
            view.indicator->setBackgroundColor(selected ? m_checkedIndicatorColor : m_uncheckedIndicatorColor);
            view.indicator->setBorder(m_borderWidth, selected ? m_checkedBorderColor : m_uncheckedBorderColor);
        }

        if (view.dot) {
            view.dot->setBackgroundColor(m_checkedBorderColor);
            view.dot->setDisplay(selected ? YGDisplayFlex : YGDisplayNone);
        }

        if (view.label) {
            view.label->setFontSize(m_fontSize);
            view.label->setText(m_options[i]);
            view.label->setTextColor(selected ? m_checkedTextColor : m_uncheckedTextColor);
            view.label->setFlexGrow(m_optionDirection == LFOrientation::Vertical ? 1.0f : 0.0f);
        }
    }
}

void LFRadioGroup::updateItemGeometry(OptionView& view) {
    if (view.indicator) {
        view.indicator->setWidth(m_indicatorSize);
        view.indicator->setHeight(m_indicatorSize);
        view.indicator->setBorderRadius(m_indicatorSize * 0.5f);
    }

    if (view.dot) {
        const float dotSize = std::min(m_innerDotSize, m_indicatorSize);
        view.dot->setWidth(dotSize);
        view.dot->setHeight(dotSize);
        view.dot->setBorderRadius(dotSize * 0.5f);
    }
}

bool LFRadioGroup::isValidIndex(int index) const {
    return index >= 0 && index < static_cast<int>(m_options.size());
}
