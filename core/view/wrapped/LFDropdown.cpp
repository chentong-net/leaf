//
// Created by Chen Tong on 2026/5/30.
// UI Component - Dropdown Implementation
//

#include "LFDropdown.h"

#include <algorithm>
#include <cmath>
#include "LFEngine.h"
#include "event/LFEvent.h"
#include "view/base/LFOverlay.h"

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
    m_triggerButton->setBackgroundColor(LFButtonState::Normal, m_triggerNormalColor);
    m_triggerButton->setBackgroundColor(LFButtonState::Pressed, m_triggerPressedColor);
    m_triggerButton->setBorder(1.0f, m_borderColor);
    m_triggerButton->setBorderRadius(m_cornerRadius);

    std::weak_ptr<LFDropdown> weakSelf = std::static_pointer_cast<LFDropdown>(shared_from_this());
    m_triggerButton->setOnClick([weakSelf](LFButton*) {
        if (auto self = weakSelf.lock()) {
            self->toggle();
        }
    });

    addChild(m_triggerButton);
    updateClickEffects();

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
    m_scrollView->setPreventAncestorScroll(true);

    m_optionsContainer = LFLinear::createVertical();
    m_optionsContainer->matchParentWidth();
    m_optionsContainer->wrapContentHeight();

    m_scrollView->addChild(m_optionsContainer);
    m_panelContainer->addChild(m_scrollView, LFBoxAlign::MatchParent);
    addChild(m_panelContainer);

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

void LFDropdown::setDisplayMode(LFDropdownDisplayMode mode) {
    if (m_displayMode == mode) return;

    if (m_displayMode == LFDropdownDisplayMode::Popup) {
        hidePopupPanel();
    }

    m_displayMode = mode;

    if (m_displayMode == LFDropdownDisplayMode::Inline) {
        ensureInlinePanelAttached();
    }

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

void LFDropdown::enableScale(bool enabled) {
    if (m_scaleEnabled == enabled) return;
    m_scaleEnabled = enabled;
    updateClickEffects();
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
    button->setFontSize(m_fontSize);
    button->setTextColor(m_textColor);
    button->setBackgroundColor(LFButtonState::Normal, m_optionNormalColor);
    button->setBackgroundColor(LFButtonState::Pressed, m_optionPressedColor);
    button->setClickEffect(m_scaleEnabled ? LFClickEffect::Scale : LFClickEffect::None);

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

    if (m_displayMode == LFDropdownDisplayMode::Popup) {
        if (m_expanded && !m_options.empty()) {
            showPopupPanel();
        } else {
            hidePopupPanel();
        }
        updateTriggerText();
        return;
    }

    ensureInlinePanelAttached();
    if (m_expanded && !m_options.empty()) {
        m_panelContainer->setHeight(resolvePanelHeight());
        m_panelContainer->setDisplay(YGDisplayFlex);
    } else {
        m_panelContainer->setDisplay(YGDisplayNone);
    }
    updateTriggerText();
}

void LFDropdown::updateClickEffects() {
    LFClickEffect effect = m_scaleEnabled ? LFClickEffect::Scale : LFClickEffect::None;

    if (m_triggerButton) {
        m_triggerButton->setClickEffect(effect);
    }

    for (auto& button : m_optionButtons) {
        if (button) {
            button->setClickEffect(effect);
        }
    }
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

void LFDropdown::ensureInlinePanelAttached() {
    if (!m_panelContainer) return;

    if (m_panelContainer->getParent() != this) {
        m_panelContainer->removeFromParent();
        m_panelContainer->setPositionType(YGPositionTypeRelative);
        m_panelContainer->setPosition(YGEdgeLeft, YGUndefined);
        m_panelContainer->setPosition(YGEdgeTop, YGUndefined);
        m_panelContainer->setPosition(YGEdgeRight, YGUndefined);
        m_panelContainer->setPosition(YGEdgeBottom, YGUndefined);
        m_panelContainer->setTranslate(0.0f, 0.0f);
        m_panelContainer->setTranslatePercent(0.0f, 0.0f);
        m_panelContainer->matchParentWidth();
        LFNode::addChild(m_panelContainer);
    }
}

void LFDropdown::ensurePopupOverlay() {
    if (m_popupOverlay) return;

    m_popupOverlay = LFOverlay::create();
    m_popupOverlay->setVisible(false);
    m_popupOverlay->setModal(true);
    m_popupOverlay->setBarrierColor(0x00000000);
    m_popupOverlay->setDismissOnBarrierTap(true);

    std::weak_ptr<LFDropdown> weakSelf = std::static_pointer_cast<LFDropdown>(shared_from_this());
    m_popupOverlay->setOnDismiss([weakSelf]() {
        if (auto self = weakSelf.lock()) {
            self->handlePopupDismissed();
        }
    });
}

void LFDropdown::showPopupPanel() {
    if (!m_panelContainer || m_options.empty()) return;

    ensurePopupOverlay();
    if (!m_popupOverlay) return;

    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    resolvePopupGeometry(x, y, width, height);

    if (!almostEqual(m_popupResolvedX, x) ||
        !almostEqual(m_popupResolvedY, y) ||
        !almostEqual(m_popupResolvedWidth, width) ||
        !almostEqual(m_popupResolvedHeight, height)) {
        m_popupResolvedX = x;
        m_popupResolvedY = y;
        m_popupResolvedWidth = width;
        m_popupResolvedHeight = height;
        m_panelContainer->setWidth(width);
        m_panelContainer->setHeight(height);
    }

    m_panelContainer->setDisplay(YGDisplayFlex);
    m_popupOverlay->show(m_panelContainer, LFBoxAlign::TopLeft, x, y);
}

void LFDropdown::hidePopupPanel() {
    if (m_popupOverlay && m_popupOverlay->isShowing()) {
        m_popupOverlay->dismiss();
    }

    if (m_popupOverlay && m_popupOverlay->getParent()) {
        m_popupOverlay->removeFromParent();
    }

    if (m_panelContainer) {
        m_panelContainer->setDisplay(YGDisplayNone);
    }
}

void LFDropdown::handlePopupDismissed() {
    if (m_panelContainer) {
        m_panelContainer->setDisplay(YGDisplayNone);
    }

    if (m_popupOverlay && m_popupOverlay->getParent()) {
        m_popupOverlay->removeFromParent();
    }

    if (m_expanded) {
        m_expanded = false;
        updateTriggerText();
    }
}

float LFDropdown::resolvePanelHeight() const {
    float desiredHeight = static_cast<float>(m_options.size()) * m_optionHeight;
    return std::min(desiredHeight, m_maxPanelHeight);
}

float LFDropdown::resolvePopupPanelWidth() const {
    if (m_triggerButton && m_triggerButton->getLayoutWidth() > 0.0f) {
        return m_triggerButton->getLayoutWidth();
    }
    if (getLayoutWidth() > 0.0f) {
        return getLayoutWidth();
    }
    return 200.0f;
}

void LFDropdown::resolvePopupGeometry(float& x, float& y, float& width, float& height) const {
    width = resolvePopupPanelWidth();
    height = resolvePanelHeight();

    float triggerX = m_triggerButton ? getAbsoluteX(m_triggerButton.get()) : getAbsoluteX(this);
    float triggerY = m_triggerButton ? getAbsoluteY(m_triggerButton.get()) : getAbsoluteY(this);
    float triggerHeight = m_triggerButton ? m_triggerButton->getLayoutHeight() : getLayoutHeight();

    x = triggerX;
    y = triggerY + triggerHeight + m_popupGap;

    auto& engine = LFEngine::getInstance();
    float windowWidth = engine.getWindowWidth();
    float windowHeight = engine.getWindowHeight();

    if (windowWidth > 0.0f) {
        width = std::min(width, windowWidth);
        if (x + width > windowWidth) {
            x = std::max(0.0f, windowWidth - width);
        }
        x = std::max(0.0f, x);
    }

    if (windowHeight > 0.0f && y + height > windowHeight) {
        float upwardY = triggerY - height - m_popupGap;
        if (upwardY >= 0.0f) {
            y = upwardY;
        } else {
            height = std::max(0.0f, windowHeight - y);
        }
    }
}

float LFDropdown::getAbsoluteX(const LFNode* node) const {
    float x = 0.0f;
    const LFNode* current = node;
    while (current) {
        x += current->getLayoutX();
        const auto& transform = current->getTransform();
        x += transform.translateX;
        x += current->getLayoutWidth() * (transform.translatePercentX / 100.0f);
        current = current->getParent();
    }
    return x;
}

float LFDropdown::getAbsoluteY(const LFNode* node) const {
    float y = 0.0f;
    const LFNode* current = node;
    while (current) {
        y += current->getLayoutY();
        const auto& transform = current->getTransform();
        y += transform.translateY;
        y += current->getLayoutHeight() * (transform.translatePercentY / 100.0f);
        current = current->getParent();
    }
    return y;
}

bool LFDropdown::almostEqual(float a, float b) {
    return std::fabs(a - b) <= 0.001f;
}
