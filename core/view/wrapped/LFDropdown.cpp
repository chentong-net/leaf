//
// Created by Chen Tong on 2026/5/30.
// UI Component - Dropdown Implementation
//

#include "LFDropdown.h"

#include <algorithm>
#include "event/LFEvent.h"

LFDropdown::Ptr LFDropdown::create() {
    auto dropdown = std::make_shared<LFDropdown>();
    dropdown->initLayout();
    return dropdown;
}

LFDropdown::Ptr LFDropdown::create(const std::vector<std::string>& options) {
    auto dropdown = create();
    dropdown->setOptions(options);
    return dropdown;
}

LFDropdown::LFDropdown() {
    setOrientation(LFOrientation::Vertical);
    setSpacing(4.0f);
    wrapContentHeight();
}

void LFDropdown::initLayout() {
    matchParentWidth();
    m_triggerButton = LFButton::create();
    m_triggerButton->matchParentWidth();
    m_triggerButton->setHeight(44.0f);
    m_triggerButton->setClickEffect(LFClickEffect::None);
    m_triggerButton->setBackgroundColor(LFButtonState::Normal, m_triggerNormalColor);
    m_triggerButton->setBackgroundColor(LFButtonState::Pressed, m_triggerPressedColor);
    m_triggerButton->setBorder(1.0f, m_borderColor);
    m_triggerButton->setBorderRadius(m_cornerRadius);

    std::weak_ptr<LFDropdown> weakSelf = std::static_pointer_cast<LFDropdown>(shared_from_this());
    m_triggerButton->setOnClick([weakSelf](LFButton*) {
        if (auto self = weakSelf.lock()) {
            self->requestFocus();
            self->toggle();
        }
    });

    addChild(m_triggerButton);

    m_panelContainer = LFBox::create();
    m_panelContainer->matchParentWidth();
    m_panelContainer->setBackgroundColor(m_panelBackgroundColor);
    m_panelContainer->setBorder(1.0f, m_borderColor);
    m_panelContainer->setBorderRadius(m_cornerRadius);
    m_panelContainer->setMasksToBounds(true);
    m_panelContainer->setDisplay(YGDisplayNone);

    m_scrollView = LFScrollView::createVertical();
    m_scrollView->matchParentWidth();
    m_scrollView->matchParentHeight();
    m_scrollView->setBounces(false);

    m_optionsContainer = LFLinear::createVertical();
    m_optionsContainer->matchParentWidth();
    m_optionsContainer->wrapContentHeight();

    m_scrollView->addChild(m_optionsContainer);
    m_panelContainer->addChild(m_scrollView, LFBoxAlign::MatchParent);
    addChild(m_panelContainer);

    setFocusable(true);
    setOnKeyDown([weakSelf](LFKeyEvent& event) {
        auto self = weakSelf.lock();
        if (!self || !self->isEnabled()) return;

        if (event.keyCode == LFKeyCode::Enter) {
            self->toggle();
            event.stopPropagation();
        } else if (event.keyCode == LFKeyCode::Escape) {
            self->collapse();
            event.stopPropagation();
        } else if (event.keyCode == LFKeyCode::Down) {
            int next = self->getSelectedIndex() + 1;
            if (next < 0) next = 0;
            if (next < static_cast<int>(self->getOptions().size())) {
                self->setSelectedIndex(next);
                self->expand();
            }
            event.stopPropagation();
        } else if (event.keyCode == LFKeyCode::Up) {
            int prev = self->getSelectedIndex() - 1;
            if (prev >= 0) {
                self->setSelectedIndex(prev);
                self->expand();
            }
            event.stopPropagation();
        }
    });

    updateTriggerText();
}

void LFDropdown::setOptions(const std::vector<std::string>& options) {
    m_options = options;
    if (m_selectedIndex >= static_cast<int>(m_options.size())) {
        m_selectedIndex = -1;
    }
    rebuildOptions();
    updateTriggerText();
    updateExpandedState();
}

void LFDropdown::setSelectedIndex(int index, bool notify) {
    if (index < -1 || index >= static_cast<int>(m_options.size())) return;
    if (m_selectedIndex == index) return;

    m_selectedIndex = index;
    updateTriggerText();
    updateOptionStyles();

    if (notify && m_onSelectionChanged && m_selectedIndex >= 0) {
        m_onSelectionChanged(m_selectedIndex, m_options[m_selectedIndex]);
    }
}

std::string LFDropdown::getSelectedValue() const {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_options.size())) {
        return "";
    }
    return m_options[m_selectedIndex];
}

void LFDropdown::setPlaceholder(const std::string& text) {
    m_placeholder = text;
    updateTriggerText();
}

void LFDropdown::setOnSelectionChanged(SelectionChangedCallback callback) {
    m_onSelectionChanged = std::move(callback);
}

void LFDropdown::setExpanded(bool expanded) {
    if (!m_enabled) expanded = false;
    if (m_expanded == expanded) return;
    m_expanded = expanded;
    updateExpandedState();
}

void LFDropdown::toggle() {
    setExpanded(!m_expanded);
}

void LFDropdown::collapse() {
    setExpanded(false);
}

void LFDropdown::expand() {
    setExpanded(true);
}

void LFDropdown::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    if (!m_enabled) {
        m_expanded = false;
    }
    if (m_triggerButton) {
        m_triggerButton->setEnabled(enabled);
    }
    for (auto& button : m_optionButtons) {
        if (button) {
            button->setEnabled(enabled);
        }
    }
    updateExpandedState();
}

void LFDropdown::setOptionHeight(float height) {
    if (height <= 0.0f) return;
    m_optionHeight = height;
    rebuildOptions();
    updateExpandedState();
}

void LFDropdown::setMaxPanelHeight(float height) {
    if (height <= 0.0f) return;
    m_maxPanelHeight = height;
    updateExpandedState();
}

void LFDropdown::setFontSize(float size) {
    if (size <= 0.0f) return;
    m_fontSize = size;
    updateTriggerText();
    updateOptionStyles();
}

void LFDropdown::setTextColor(uint32_t color) {
    m_textColor = color;
    updateTriggerText();
    updateOptionStyles();
}

void LFDropdown::setSelectedTextColor(uint32_t color) {
    m_selectedTextColor = color;
    updateOptionStyles();
}

void LFDropdown::setPlaceholderColor(uint32_t color) {
    m_placeholderColor = color;
    updateTriggerText();
}

void LFDropdown::setTriggerBackgroundColor(uint32_t normal, uint32_t pressed) {
    m_triggerNormalColor = normal;
    m_triggerPressedColor = pressed;
    if (!m_triggerButton) return;
    m_triggerButton->setBackgroundColor(LFButtonState::Normal, normal);
    m_triggerButton->setBackgroundColor(LFButtonState::Pressed, pressed);
}

void LFDropdown::setPanelBackgroundColor(uint32_t color) {
    m_panelBackgroundColor = color;
    if (m_panelContainer) {
        m_panelContainer->setBackgroundColor(color);
    }
}

void LFDropdown::setOptionBackgroundColor(uint32_t normal, uint32_t pressed, uint32_t selected) {
    m_optionNormalColor = normal;
    m_optionPressedColor = pressed;
    m_optionSelectedColor = selected;
    updateOptionStyles();
}

void LFDropdown::setBorderColor(uint32_t color) {
    m_borderColor = color;
    if (m_triggerButton) {
        m_triggerButton->setBorder(1.0f, color);
    }
    if (m_panelContainer) {
        m_panelContainer->setBorder(1.0f, color);
    }
}

void LFDropdown::setCornerRadius(float radius) {
    if (radius < 0.0f) return;
    m_cornerRadius = radius;
    if (m_triggerButton) {
        m_triggerButton->setBorderRadius(radius);
    }
    if (m_panelContainer) {
        m_panelContainer->setBorderRadius(radius);
    }
}

void LFDropdown::rebuildOptions() {
    if (!m_optionsContainer) return;

    auto children = m_optionsContainer->getChildren();
    for (const auto& child : children) {
        if (child) {
            child->removeFromParent();
        }
    }

    m_optionButtons.clear();
    for (int i = 0; i < static_cast<int>(m_options.size()); ++i) {
        auto optionButton = createOptionButton(i);
        m_optionsContainer->addChild(optionButton);
        m_optionButtons.push_back(optionButton);
    }

    updateOptionStyles();
}

LFButton::Ptr LFDropdown::createOptionButton(int index) {
    auto button = LFButton::create(m_options[index]);
    button->matchParentWidth();
    button->setHeight(m_optionHeight);
    button->setClickEffect(LFClickEffect::None);
    button->setFontSize(m_fontSize);
    button->setTextColor(m_textColor);
    button->setBackgroundColor(LFButtonState::Normal, m_optionNormalColor);
    button->setBackgroundColor(LFButtonState::Pressed, m_optionPressedColor);

    std::weak_ptr<LFDropdown> weakSelf = std::static_pointer_cast<LFDropdown>(shared_from_this());
    button->setOnClick([weakSelf, index](LFButton*) {
        if (auto self = weakSelf.lock()) {
            self->setSelectedIndex(index);
            self->collapse();
        }
    });

    return button;
}

void LFDropdown::updateTriggerText() {
    if (!m_triggerButton) return;

    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_options.size())) {
        m_triggerButton->setText(m_options[m_selectedIndex] + (m_expanded ? "  ▲" : "  ▼"));
        m_triggerButton->setTextColor(m_textColor);
    } else {
        m_triggerButton->setText(m_placeholder + (m_expanded ? "  ▲" : "  ▼"));
        m_triggerButton->setTextColor(m_placeholderColor);
    }
    m_triggerButton->setFontSize(m_fontSize);
}

void LFDropdown::updateExpandedState() {
    if (!m_panelContainer) return;

    if (m_expanded && !m_options.empty()) {
        m_panelContainer->setHeight(resolvePanelHeight());
        m_panelContainer->setDisplay(YGDisplayFlex);
    } else {
        m_panelContainer->setDisplay(YGDisplayNone);
    }
    updateTriggerText();
}

void LFDropdown::updateOptionStyles() {
    for (int i = 0; i < static_cast<int>(m_optionButtons.size()); ++i) {
        auto& button = m_optionButtons[i];
        if (!button) continue;

        bool selected = i == m_selectedIndex;
        uint32_t normalColor = selected ? m_optionSelectedColor : m_optionNormalColor;
        uint32_t textColor = selected ? m_selectedTextColor : m_textColor;

        button->setFontSize(m_fontSize);
        button->setTextColor(textColor);
        button->setBackgroundColor(LFButtonState::Normal, normalColor);
        button->setBackgroundColor(LFButtonState::Pressed, m_optionPressedColor);
    }
}

float LFDropdown::resolvePanelHeight() const {
    float desiredHeight = static_cast<float>(m_options.size()) * m_optionHeight;
    return std::min(desiredHeight, m_maxPanelHeight);
}
