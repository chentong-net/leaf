//
// Created by Chen Tong on 2026/6/2.
// UI Plugin - Radio Group
//

#ifndef LEAF_LFRADIOGROUP_H
#define LEAF_LFRADIOGROUP_H

#include "view/base/LFText.h"
#include "view/layout/LFBox.h"
#include "view/layout/LFLinear.h"

class LFRadioGroup : public LFLinear {
public:
    using Ptr = std::shared_ptr<LFRadioGroup>;
    using SelectionChangedCallback = std::function<void(int index, const std::string& value)>;

    static Ptr create();
    static Ptr create(const std::vector<std::string>& options);

    LFRadioGroup();
    ~LFRadioGroup() override = default;

    void setOptions(const std::vector<std::string>& options);
    const std::vector<std::string>& getOptions() const { return m_options; }

    void setSelectedIndex(int index, bool notify = true);
    int getSelectedIndex() const { return m_selectedIndex; }
    std::string getSelectedValue() const;
    void clearSelection(bool notify = true);

    void setOnSelectionChanged(SelectionChangedCallback callback);

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void setOptionDirection(LFOrientation orientation);
    LFOrientation getOptionDirection() const { return m_optionDirection; }

    void setOptionSpacing(float spacing);
    float getOptionSpacing() const { return m_optionSpacing; }

    void setItemSpacing(float spacing);
    float getItemSpacing() const { return m_itemSpacing; }

    void setIndicatorSize(float size);
    float getIndicatorSize() const { return m_indicatorSize; }

    void setInnerDotSize(float size);
    float getInnerDotSize() const { return m_innerDotSize; }

    void setBorderWidth(float width);
    float getBorderWidth() const { return m_borderWidth; }

    void setFontSize(float size);
    float getFontSize() const { return m_fontSize; }

    void setTextColor(uint32_t uncheckedColor, uint32_t checkedColor);
    void setIndicatorColor(uint32_t uncheckedColor, uint32_t checkedColor);
    void setBorderColor(uint32_t uncheckedColor, uint32_t checkedColor);
    void setItemBackgroundColor(uint32_t uncheckedColor, uint32_t checkedColor);
    void setDisabledOpacity(float opacity);

private:
    struct OptionView {
        std::shared_ptr<LFLinear> row;
        LFBox::Ptr indicator;
        LFBox::Ptr dot;
        std::shared_ptr<LFText> label;
    };

    void initLayout();
    void rebuildOptions();
    void updateVisualState();
    void updateItemGeometry(OptionView& view);
    bool isValidIndex(int index) const;

    std::vector<std::string> m_options;
    std::vector<OptionView> m_optionViews;
    SelectionChangedCallback m_onSelectionChanged;

    int m_selectedIndex = -1;
    bool m_enabled = true;
    LFOrientation m_optionDirection = LFOrientation::Vertical;

    float m_optionSpacing = 10.0f;
    float m_itemSpacing = 10.0f;
    float m_indicatorSize = 20.0f;
    float m_innerDotSize = 10.0f;
    float m_borderWidth = 1.5f;
    float m_fontSize = 15.0f;
    float m_disabledOpacity = 0.55f;

    uint32_t m_uncheckedTextColor = 0xFF334155;
    uint32_t m_checkedTextColor = 0xFF0F5B99;
    uint32_t m_uncheckedIndicatorColor = 0xFFFFFFFF;
    uint32_t m_checkedIndicatorColor = 0xFFE8F2FF;
    uint32_t m_uncheckedBorderColor = 0xFFCBD5E1;
    uint32_t m_checkedBorderColor = 0xFF2563EB;
    uint32_t m_uncheckedItemBackgroundColor = 0x00000000;
    uint32_t m_checkedItemBackgroundColor = 0x0F2563EB;
};

#endif // LEAF_LFRADIOGROUP_H
