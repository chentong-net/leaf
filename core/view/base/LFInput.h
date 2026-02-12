//
// Created by Chen Tong on 2026/2/12.
//

#ifndef LEAF_LFINPUT_H
#define LEAF_LFINPUT_H

#include "view/base/LFNode.h"

/**
 * 单行输入框组件
 */
class LFInput : public LFNode {
public:
    using Ptr = std::shared_ptr<LFInput>;
    using TextChangeCallback = std::function<void(const std::string&)>;
    using SubmitCallback = std::function<void(const std::string&)>;

    LFInput();
    virtual ~LFInput() = default;

    static Ptr create();

    void setText(const std::string& text);
    const std::string& getText() const { return m_text; }

    void setPlaceholder(const std::string& placeholder);
    void setFontSize(float fontSize);
    void setFontFamily(const std::string& fontFamily);
    void setTextColor(uint32_t color);
    void setPlaceholderColor(uint32_t color);
    void setCursorColor(uint32_t color);
    void setMaxLength(size_t maxLength);
    void setPasswordMode(bool enabled);

    void setOnChange(TextChangeCallback callback);
    void setOnSubmit(SubmitCallback callback);

protected:
    void onDrawContent(NVGcontext* vg) override;
    void onFocusChanged(bool focused) override;

private:
    void setupEvents();

    void insertText(const std::string& utf8);
    void deleteBackward();
    void deleteForward();
    void moveCursorLeft();
    void moveCursorRight();
    void moveCursorToStart();
    void moveCursorToEnd();
    void commitTextChanged();

    std::string getDisplayText() const;
    size_t cursorCodepointIndex() const;
    float measureTextWidth(const std::string& text) const;
    float measureCursorOffset() const;
    void ensureCursorVisible(float contentWidth);

    static size_t prevUtf8Index(const std::string& text, size_t index);
    static size_t nextUtf8Index(const std::string& text, size_t index);
    static size_t utf8CodepointCount(const std::string& text);
    static size_t utf8CodepointCountUntilByte(const std::string& text, size_t byteIndex);
    static std::string utf8FromCodepoint(uint32_t codepoint);

    std::string m_text;
    std::string m_placeholder;
    std::string m_fontFamily = "sans";
    float m_fontSize = 16.0f;
    uint32_t m_textColor = 0xFF111111;
    uint32_t m_placeholderColor = 0xFF999999;
    uint32_t m_cursorColor = 0xFF222222;
    size_t m_maxLength = 0; // 0表示不限制
    size_t m_cursorIndex = 0; // UTF-8字节索引
    float m_scrollX = 0.0f;
    bool m_passwordMode = false;

    TextChangeCallback m_onChange;
    SubmitCallback m_onSubmit;
};

#endif // LEAF_LFINPUT_H
