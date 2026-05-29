//
// Created by Chen Tong on 2026/5/30.
// UI Component - Dropdown
//

#ifndef LEAF_LFDROPDOWN_H
#define LEAF_LFDROPDOWN_H

#include "LFButton.h"
#include "view/layout/LFBox.h"
#include "LFScrollView.h"
#include "view/layout/LFLinear.h"

/**
 * Inline dropdown component.
 *
 * The first version expands in normal layout flow because Leaf does not have a
 * global overlay layer yet. Once Popup/Overlay is available, the option panel
 * can be moved out without changing the public selection API.
 */
class LFDropdown : public LFLinear {
public:
    using Ptr = std::shared_ptr<LFDropdown>;
    using SelectionChangedCallback = std::function<void(int index, const std::string& value)>;

    static Ptr create();
    static Ptr create(const std::vector<std::string>& options);

    LFDropdown();
    virtual ~LFDropdown() = default;

    void setOptions(const std::vector<std::string>& options);
    const std::vector<std::string>& getOptions() const { return m_options; }

    void setSelectedIndex(int index, bool notify = true);
    int getSelectedIndex() const { return m_selectedIndex; }
    std::string getSelectedValue() const;

    void setPlaceholder(const std::string& text);
    void setOnSelectionChanged(SelectionChangedCallback callback);

    void setExpanded(bool expanded);
    bool isExpanded() const { return m_expanded; }
    void toggle();
    void collapse();
    void expand();

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void setOptionHeight(float height);
    void setMaxPanelHeight(float height);
    void setFontSize(float size);
    void setTextColor(uint32_t color);
    void setSelectedTextColor(uint32_t color);
    void setPlaceholderColor(uint32_t color);
    void setTriggerBackgroundColor(uint32_t normal, uint32_t pressed);
    void setPanelBackgroundColor(uint32_t color);
    void setOptionBackgroundColor(uint32_t normal, uint32_t pressed, uint32_t selected);
    void setBorderColor(uint32_t color);
    void setCornerRadius(float radius);

    LFButton::Ptr getTriggerButton() const { return m_triggerButton; }
    LFBox::Ptr getPanelContainer() const { return m_panelContainer; }

private:
    void initLayout();
    void rebuildOptions();
    void updateTriggerText();
    void updateExpandedState();
    void updateOptionStyles();
    LFButton::Ptr createOptionButton(int index);
    float resolvePanelHeight() const;

    std::vector<std::string> m_options;
    std::vector<LFButton::Ptr> m_optionButtons;

    LFButton::Ptr m_triggerButton;
    LFBox::Ptr m_panelContainer;
    LFScrollView::Ptr m_scrollView;
    std::shared_ptr<LFLinear> m_optionsContainer;

    SelectionChangedCallback m_onSelectionChanged;

    std::string m_placeholder = "Select";
    int m_selectedIndex = -1;
    bool m_expanded = false;
    bool m_enabled = true;

    float m_optionHeight = 44.0f;
    float m_maxPanelHeight = 220.0f;
    float m_fontSize = 15.0f;
    float m_cornerRadius = 8.0f;

    uint32_t m_textColor = 0xFF222222;
    uint32_t m_selectedTextColor = 0xFF0F5B99;
    uint32_t m_placeholderColor = 0xFF888888;
    uint32_t m_triggerNormalColor = 0xFFFFFFFF;
    uint32_t m_triggerPressedColor = 0xFFF3F6FA;
    uint32_t m_panelBackgroundColor = 0xFFFFFFFF;
    uint32_t m_optionNormalColor = 0xFFFFFFFF;
    uint32_t m_optionPressedColor = 0xFFF3F6FA;
    uint32_t m_optionSelectedColor = 0xFFE8F2FF;
    uint32_t m_borderColor = 0xFFD8DEE8;
};

#endif // LEAF_LFDROPDOWN_H
