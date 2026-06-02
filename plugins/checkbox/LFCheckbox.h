//
// Created by Chen Tong on 2026/6/2.
// UI Plugin - Checkbox
//

#ifndef LEAF_LFCHECKBOX_H
#define LEAF_LFCHECKBOX_H

#include "view/base/LFText.h"
#include "view/layout/LFBox.h"
#include "view/layout/LFLinear.h"

class LFCheckbox : public LFLinear {
public:
    using Ptr = std::shared_ptr<LFCheckbox>;
    using CheckedChangedCallback = std::function<void(bool checked)>;

    static Ptr create(const std::string& text = "", bool checked = false);

    LFCheckbox();
    ~LFCheckbox() override = default;

    void setChecked(bool checked, bool notify = true);
    bool isChecked() const { return m_checked; }
    void toggle(bool notify = true);

    void setText(const std::string& text);
    const std::string& getText() const { return m_text; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void setOnCheckedChanged(CheckedChangedCallback callback);

    void setBoxSize(float size);
    float getBoxSize() const { return m_boxSize; }

    void setCornerRadius(float radius);
    float getCornerRadius() const { return m_cornerRadius; }

    void setBorderWidth(float width);
    float getBorderWidth() const { return m_borderWidth; }

    void setFontSize(float size);
    float getFontSize() const { return m_fontSize; }

    void setLabelSpacing(float spacing);
    float getLabelSpacing() const { return m_labelSpacing; }

    void setTextColor(uint32_t uncheckedColor, uint32_t checkedColor);
    void setIndicatorColor(uint32_t uncheckedColor, uint32_t checkedColor);
    void setBorderColor(uint32_t uncheckedColor, uint32_t checkedColor);
    void setCheckmarkColor(uint32_t color);
    void setDisabledOpacity(float opacity);

    LFBox::Ptr getIndicatorNode() const { return m_indicator; }
    std::shared_ptr<LFText> getLabelNode() const { return m_label; }

private:
    void initLayout();
    void initInteractions();
    void updateVisualState();
    void updateIndicatorGeometry();

    LFBox::Ptr m_indicator;
    LFBox::Ptr m_checkmarkContainer;
    LFBox::Ptr m_checkmarkShortStroke;
    LFBox::Ptr m_checkmarkLongStroke;
    std::shared_ptr<LFText> m_label;
    CheckedChangedCallback m_onCheckedChanged;

    std::string m_text;
    bool m_checked = false;
    bool m_enabled = true;

    float m_boxSize = 22.0f;
    float m_cornerRadius = 6.0f;
    float m_borderWidth = 1.5f;
    float m_fontSize = 15.0f;
    float m_labelSpacing = 10.0f;
    float m_disabledOpacity = 0.55f;

    uint32_t m_uncheckedTextColor = 0xFF334155;
    uint32_t m_checkedTextColor = 0xFF0F5B99;
    uint32_t m_uncheckedIndicatorColor = 0xFFFFFFFF;
    uint32_t m_checkedIndicatorColor = 0xFF2563EB;
    uint32_t m_uncheckedBorderColor = 0xFFCBD5E1;
    uint32_t m_checkedBorderColor = 0xFF2563EB;
    uint32_t m_checkmarkColor = 0xFFFFFFFF;
};

#endif // LEAF_LFCHECKBOX_H
