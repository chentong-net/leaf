//
// Created by Chen Tong on 2026/2/12.
//

#include "view/base/LFInput.h"
#include "LFEngine.h"

namespace {

bool isUtf8ContinuationByte(unsigned char c) {
    return (c & 0xC0) == 0x80;
}

}

LFInput::LFInput() {
    setFocusable(true);
    setTouchEnabled(true);

    setWidth(220.0f);
    setHeight(40.0f);
    setPadding(YGEdgeHorizontal, 12.0f);
    setPadding(YGEdgeVertical, 8.0f);
    setBackgroundColor(0xFFFFFFFF);
    setBorder(1.0f, 0xFFDDDDDD);
    setBorderRadius(8.0f);

    setupEvents();
}

LFInput::Ptr LFInput::create() {
    return std::make_shared<LFInput>();
}

void LFInput::setText(const std::string& text) {
    if (m_text == text) return;
    m_text = text;
    if (m_cursorIndex > m_text.size()) {
        m_cursorIndex = m_text.size();
    }
    commitTextChanged();
}

void LFInput::setPlaceholder(const std::string& placeholder) {
    if (m_placeholder == placeholder) return;
    m_placeholder = placeholder;
    markDirty();
}

void LFInput::setFontSize(float fontSize) {
    float safeSize = std::max(1.0f, fontSize);
    if (m_fontSize == safeSize) return;
    m_fontSize = safeSize;
    markDirty();
}

void LFInput::setFontFamily(const std::string& fontFamily) {
    if (m_fontFamily == fontFamily) return;
    m_fontFamily = fontFamily;
    markDirty();
}

void LFInput::setTextColor(uint32_t color) {
    if (m_textColor == color) return;
    m_textColor = color;
    markDirty();
}

void LFInput::setPlaceholderColor(uint32_t color) {
    if (m_placeholderColor == color) return;
    m_placeholderColor = color;
    markDirty();
}

void LFInput::setCursorColor(uint32_t color) {
    if (m_cursorColor == color) return;
    m_cursorColor = color;
    markDirty();
}

void LFInput::setMaxLength(size_t maxLength) {
    if (m_maxLength == maxLength) return;
    m_maxLength = maxLength;

    if (m_maxLength > 0 && utf8CodepointCount(m_text) > m_maxLength) {
        std::string newText;
        newText.reserve(m_text.size());
        size_t index = 0;
        size_t count = 0;
        while (index < m_text.size() && count < m_maxLength) {
            size_t next = nextUtf8Index(m_text, index);
            if (next <= index) break;
            newText.append(m_text, index, next - index);
            index = next;
            ++count;
        }
        m_text = newText;
        if (m_cursorIndex > m_text.size()) {
            m_cursorIndex = m_text.size();
        }
        commitTextChanged();
    } else {
        markDirty();
    }
}

void LFInput::setPasswordMode(bool enabled) {
    if (m_passwordMode == enabled) return;
    m_passwordMode = enabled;
    markDirty();
}

void LFInput::setOnChange(TextChangeCallback callback) {
    m_onChange = callback;
}

void LFInput::setOnSubmit(SubmitCallback callback) {
    m_onSubmit = callback;
}

void LFInput::setupEvents() {
    setOnTouchDown([this](const LFTouchEvent&) {
        requestFocus();
        moveCursorToEnd();
        markDirty();
    });

    setOnCharInput([this](LFKeyEvent& event) {
        if (!hasFocus()) return;

        if (event.codepoint == 0) return;
        if (event.codepoint < 32) return;

        if ((event.modifiers & LFKeyModCtrl) || (event.modifiers & LFKeyModAlt) || (event.modifiers & LFKeyModSuper)) {
            return;
        }

        std::string utf8 = utf8FromCodepoint(event.codepoint);
        if (!utf8.empty()) {
            insertText(utf8);
        }
    });

    setOnKeyDown([this](LFKeyEvent& event) {
        if (!hasFocus()) return;

        switch (event.keyCode) {
            case LFKeyCode::Backspace:
                deleteBackward();
                break;
            case LFKeyCode::Delete:
                deleteForward();
                break;
            case LFKeyCode::Left:
                moveCursorLeft();
                break;
            case LFKeyCode::Right:
                moveCursorRight();
                break;
            case LFKeyCode::Home:
                moveCursorToStart();
                break;
            case LFKeyCode::End:
                moveCursorToEnd();
                break;
            case LFKeyCode::Enter:
                if (m_onSubmit) {
                    m_onSubmit(m_text);
                }
                break;
            case LFKeyCode::Escape:
                clearFocus();
                break;
            default:
                break;
        }
    });
}

void LFInput::insertText(const std::string& utf8) {
    if (utf8.empty()) return;

    if (m_maxLength > 0) {
        size_t currentCount = utf8CodepointCount(m_text);
        size_t appendCount = utf8CodepointCount(utf8);
        if (currentCount + appendCount > m_maxLength) {
            return;
        }
    }

    m_text.insert(m_cursorIndex, utf8);
    m_cursorIndex += utf8.size();
    commitTextChanged();
}

void LFInput::deleteBackward() {
    if (m_cursorIndex == 0 || m_text.empty()) return;
    size_t prev = prevUtf8Index(m_text, m_cursorIndex);
    if (prev >= m_cursorIndex) return;
    m_text.erase(prev, m_cursorIndex - prev);
    m_cursorIndex = prev;
    commitTextChanged();
}

void LFInput::deleteForward() {
    if (m_cursorIndex >= m_text.size() || m_text.empty()) return;
    size_t next = nextUtf8Index(m_text, m_cursorIndex);
    if (next <= m_cursorIndex) return;
    m_text.erase(m_cursorIndex, next - m_cursorIndex);
    commitTextChanged();
}

void LFInput::moveCursorLeft() {
    if (m_cursorIndex == 0) return;
    m_cursorIndex = prevUtf8Index(m_text, m_cursorIndex);
    markDirty();
}

void LFInput::moveCursorRight() {
    if (m_cursorIndex >= m_text.size()) return;
    m_cursorIndex = nextUtf8Index(m_text, m_cursorIndex);
    markDirty();
}

void LFInput::moveCursorToStart() {
    if (m_cursorIndex == 0) return;
    m_cursorIndex = 0;
    markDirty();
}

void LFInput::moveCursorToEnd() {
    size_t endIndex = m_text.size();
    if (m_cursorIndex == endIndex) return;
    m_cursorIndex = endIndex;
    markDirty();
}

void LFInput::commitTextChanged() {
    if (m_cursorIndex > m_text.size()) {
        m_cursorIndex = m_text.size();
    }
    if (m_onChange) {
        m_onChange(m_text);
    }
    markDirty();
}

std::string LFInput::getDisplayText() const {
    if (!m_passwordMode) {
        return m_text;
    }
    return std::string(utf8CodepointCount(m_text), '*');
}

size_t LFInput::cursorCodepointIndex() const {
    return utf8CodepointCountUntilByte(m_text, m_cursorIndex);
}

float LFInput::measureTextWidth(const std::string& text) const {
    if (text.empty()) return 0.0f;
    NVGcontext* vg = LFEngine::getInstance().getNVGContext();
    if (!vg) return 0.0f;

    float bounds[4];
    nvgSave(vg);
    nvgFontSize(vg, m_fontSize);
    nvgFontFace(vg, m_fontFamily.c_str());
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgTextBounds(vg, 0, 0, text.c_str(), nullptr, bounds);
    nvgRestore(vg);

    return std::max(0.0f, bounds[2] - bounds[0]);
}

float LFInput::measureCursorOffset() const {
    if (!m_passwordMode) {
        if (m_cursorIndex == 0) return 0.0f;
        return measureTextWidth(m_text.substr(0, m_cursorIndex));
    }
    size_t count = cursorCodepointIndex();
    return measureTextWidth(std::string(count, '*'));
}

void LFInput::ensureCursorVisible(float contentWidth) {
    if (contentWidth <= 0.0f) {
        m_scrollX = 0.0f;
        return;
    }

    std::string displayText = getDisplayText();
    float textWidth = measureTextWidth(displayText);
    float cursorX = measureCursorOffset();

    if (cursorX - m_scrollX > contentWidth - 2.0f) {
        m_scrollX = cursorX - (contentWidth - 2.0f);
    } else if (cursorX - m_scrollX < 0.0f) {
        m_scrollX = cursorX;
    }

    float maxScroll = std::max(0.0f, textWidth - contentWidth);
    m_scrollX = std::clamp(m_scrollX, 0.0f, maxScroll);
}

std::pair<float, float> LFInput::localToWindow(float x, float y) const {
    float outX = x;
    float outY = y;

    const LFNode* current = this;
    while (current) {
        outX += current->getLayoutX();
        outY += current->getLayoutY();

        const LFTransform& transform = current->getTransform();
        outX += transform.translateX;
        outY += transform.translateY;

        float width = current->getLayoutWidth();
        float height = current->getLayoutHeight();
        outX += width * transform.translatePercentX / 100.0f;
        outY += height * transform.translatePercentY / 100.0f;

        current = current->getParent();
    }

    return {outX, outY};
}

size_t LFInput::prevUtf8Index(const std::string& text, size_t index) {
    if (index == 0 || text.empty()) return 0;
    size_t pos = std::min(index, text.size());
    --pos;
    while (pos > 0 && isUtf8ContinuationByte((unsigned char)text[pos])) {
        --pos;
    }
    return pos;
}

size_t LFInput::nextUtf8Index(const std::string& text, size_t index) {
    if (text.empty()) return 0;
    size_t pos = std::min(index, text.size());
    if (pos >= text.size()) return text.size();

    unsigned char c = (unsigned char)text[pos];
    if ((c & 0x80) == 0) return pos + 1;
    if ((c & 0xE0) == 0xC0) return std::min(pos + 2, text.size());
    if ((c & 0xF0) == 0xE0) return std::min(pos + 3, text.size());
    if ((c & 0xF8) == 0xF0) return std::min(pos + 4, text.size());
    return std::min(pos + 1, text.size());
}

size_t LFInput::utf8CodepointCount(const std::string& text) {
    if (text.empty()) return 0;
    size_t count = 0;
    for (size_t i = 0; i < text.size();) {
        i = nextUtf8Index(text, i);
        ++count;
    }
    return count;
}

size_t LFInput::utf8CodepointCountUntilByte(const std::string& text, size_t byteIndex) {
    size_t target = std::min(byteIndex, text.size());
    size_t count = 0;
    size_t i = 0;
    while (i < target) {
        i = nextUtf8Index(text, i);
        ++count;
    }
    return count;
}

std::string LFInput::utf8FromCodepoint(uint32_t codepoint) {
    std::string out;
    if (codepoint <= 0x7F) {
        out.push_back((char)codepoint);
    } else if (codepoint <= 0x7FF) {
        out.push_back((char)(0xC0 | ((codepoint >> 6) & 0x1F)));
        out.push_back((char)(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        out.push_back((char)(0xE0 | ((codepoint >> 12) & 0x0F)));
        out.push_back((char)(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        out.push_back((char)(0xF0 | ((codepoint >> 18) & 0x07)));
        out.push_back((char)(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back((char)(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (codepoint & 0x3F)));
    }
    return out;
}

void LFInput::onDrawContent(NVGcontext* vg) {
    float layoutW = getLayoutWidth();
    float layoutH = getLayoutHeight();

    float paddingL = YGNodeLayoutGetPadding(getYGNode(), YGEdgeLeft);
    float paddingT = YGNodeLayoutGetPadding(getYGNode(), YGEdgeTop);
    float paddingR = YGNodeLayoutGetPadding(getYGNode(), YGEdgeRight);
    float paddingB = YGNodeLayoutGetPadding(getYGNode(), YGEdgeBottom);
    float borderL = YGNodeLayoutGetBorder(getYGNode(), YGEdgeLeft);
    float borderT = YGNodeLayoutGetBorder(getYGNode(), YGEdgeTop);
    float borderR = YGNodeLayoutGetBorder(getYGNode(), YGEdgeRight);
    float borderB = YGNodeLayoutGetBorder(getYGNode(), YGEdgeBottom);

    float contentX = paddingL + borderL;
    float contentY = paddingT + borderT;
    float contentW = layoutW - contentX - paddingR - borderR;
    float contentH = layoutH - contentY - paddingB - borderB;
    if (contentW <= 0 || contentH <= 0) return;

    std::string displayText = getDisplayText();
    bool drawPlaceholder = displayText.empty();
    const std::string& drawText = drawPlaceholder ? m_placeholder : displayText;

    ensureCursorVisible(contentW);

    nvgSave(vg);
    nvgFontSize(vg, m_fontSize);
    nvgFontFace(vg, m_fontFamily.c_str());
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    float bounds[4] = {0};
    float textHeight = 0.0f;
    if (!drawText.empty()) {
        nvgTextBounds(vg, 0, 0, drawText.c_str(), nullptr, bounds);
        textHeight = std::max(0.0f, bounds[3] - bounds[1]);
    }
    float drawY = contentY + std::max(0.0f, (contentH - textHeight) * 0.5f);

    nvgIntersectScissor(vg, contentX, contentY, contentW, contentH);
    nvgFillColor(vg, colorToNVG(drawPlaceholder ? m_placeholderColor : m_textColor));
    nvgText(vg, contentX - m_scrollX, drawY, drawText.c_str(), nullptr);

    if (hasFocus()) {
        float cursorX = contentX + measureCursorOffset() - m_scrollX;
        float cursorBottom = contentY + std::max(0.0f, (contentH - m_fontSize) * 0.5f) + m_fontSize;
        auto cursorInWindow = localToWindow(cursorX, cursorBottom);
        LFEngine::getInstance().updateTextInputCursor(cursorInWindow.first, cursorInWindow.second, m_fontSize);

        double t = LFEngine::getInstance().getElapsedTime();
        bool cursorVisible = std::fmod(t, 1.0) < 0.5;
        if (cursorVisible) {
            float cursorTop = contentY + std::max(0.0f, (contentH - m_fontSize) * 0.5f);
            float cursorBottom = cursorTop + m_fontSize;

            nvgBeginPath(vg);
            nvgMoveTo(vg, cursorX, cursorTop);
            nvgLineTo(vg, cursorX, cursorBottom);
            nvgStrokeWidth(vg, 1.2f);
            nvgStrokeColor(vg, colorToNVG(m_cursorColor));
            nvgStroke(vg);
        }
    }

    nvgRestore(vg);
}

void LFInput::onFocusChanged(bool focused) {
    if (focused) {
        setBorder(1.5f, 0xFF4C9AFF);
    } else {
        setBorder(1.0f, 0xFFDDDDDD);
    }
}
