//
// Created by Chen Tong on 2026/2/12.
//

#include "view/base/LFInput.h"
#include "LFEngine.h"
#include "text/LFTextLayout.h"

namespace {

LFFontHandle resolveFont(LFRenderer* renderer, const std::string& family) {
    if (!renderer) return 0;
    LFFontHandle font = renderer->findFont(family);
    if (font == 0) {
        font = renderer->findFont("sans");
    }
    return font;
}

} // namespace

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
    m_cursorIndex = graphemeBoundaryAtOrBefore(m_text, m_cursorIndex);
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

    if (m_maxLength > 0 && graphemeCount(m_text) > m_maxLength) {
        std::string newText;
        newText.reserve(m_text.size());
        size_t index = 0;
        size_t count = 0;
        while (index < m_text.size() && count < m_maxLength) {
            size_t next = nextGraphemeIndex(m_text, index);
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
        size_t currentCount = graphemeCount(m_text);
        size_t appendCount = graphemeCount(utf8);
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
    size_t prev = prevGraphemeIndex(m_text, m_cursorIndex);
    if (prev >= m_cursorIndex) return;
    m_text.erase(prev, m_cursorIndex - prev);
    m_cursorIndex = prev;
    commitTextChanged();
}

void LFInput::deleteForward() {
    if (m_cursorIndex >= m_text.size() || m_text.empty()) return;
    size_t next = nextGraphemeIndex(m_text, m_cursorIndex);
    if (next <= m_cursorIndex) return;
    m_text.erase(m_cursorIndex, next - m_cursorIndex);
    commitTextChanged();
}

void LFInput::moveCursorLeft() {
    if (m_cursorIndex == 0) return;
    m_cursorIndex = prevGraphemeIndex(m_text, m_cursorIndex);
    markDirty();
}

void LFInput::moveCursorRight() {
    if (m_cursorIndex >= m_text.size()) return;
    m_cursorIndex = nextGraphemeIndex(m_text, m_cursorIndex);
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
    return std::string(graphemeCount(m_text), '*');
}

size_t LFInput::cursorCodepointIndex() const {
    return graphemeCountUntilByte(m_text, m_cursorIndex);
}

float LFInput::measureTextWidth(const std::string& text) const {
    if (text.empty()) return 0.0f;
    LFRenderer* renderer = LFEngine::getInstance().getRenderer();
    const LFFontHandle font = resolveFont(renderer, m_fontFamily);
    if (!renderer || font == 0) return 0.0f;

    const LFTextBounds bounds = renderer->measureTextBounds(
        font,
        m_fontSize,
        LFTextOrigin::Top,
        text
    );
    return std::max(0.0f, bounds.maxX - bounds.minX);
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
    convertLocalPointToWindow(outX, outY);
    return {outX, outY};
}

size_t LFInput::prevGraphemeIndex(const std::string& text, size_t index) {
    return LFPreviousGraphemeBoundary(text, index);
}

size_t LFInput::nextGraphemeIndex(const std::string& text, size_t index) {
    return LFNextGraphemeBoundary(text, index);
}

size_t LFInput::graphemeCount(const std::string& text) {
    return LFGraphemeCount(text);
}

size_t LFInput::graphemeCountUntilByte(const std::string& text, size_t byteIndex) {
    size_t target = std::min(byteIndex, text.size());
    size_t count = 0;
    size_t i = 0;
    while (i < target) {
        const size_t next = nextGraphemeIndex(text, i);
        if (next <= i) {
            break;
        }
        i = next;
        ++count;
    }
    return count;
}

size_t LFInput::graphemeBoundaryAtOrBefore(const std::string& text, size_t byteIndex) {
    const size_t target = std::min(byteIndex, text.size());
    size_t boundary = 0;
    while (boundary < target) {
        const size_t next = nextGraphemeIndex(text, boundary);
        if (next <= boundary || next > target) {
            break;
        }
        boundary = next;
    }
    return boundary;
}

std::string LFInput::utf8FromCodepoint(uint32_t codepoint) {
    if (codepoint > 0x10FFFF ||
        (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        return {};
    }

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
    } else {
        out.push_back((char)(0xF0 | ((codepoint >> 18) & 0x07)));
        out.push_back((char)(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back((char)(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (codepoint & 0x3F)));
    }
    return out;
}

void LFInput::onDrawContent(LFPaint& paint) {
    LFRenderer* renderer = LFEngine::getInstance().getRenderer();
    if (!renderer) return;

    const LFFontHandle font = resolveFont(renderer, m_fontFamily);
    if (font == 0) return;

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

    paint.pushState();

    float textHeight = 0.0f;
    if (!drawText.empty()) {
        const LFTextBounds bounds = renderer->measureTextBounds(
            font,
            m_fontSize,
            LFTextOrigin::Top,
            drawText
        );
        textHeight = std::max(0.0f, bounds.maxY - bounds.minY);
    }
    float drawY = contentY + std::max(0.0f, (contentH - textHeight) * 0.5f);

    paint.clipRect({contentX, contentY, contentW, contentH});

    if (!drawText.empty()) {
        LFTextDraw text;
        text.font = font;
        text.text = drawText;
        text.x = contentX - m_scrollX;
        text.y = drawY;
        text.fontSize = m_fontSize;
        text.lineHeight = 1.0f;
        text.color = drawPlaceholder ? m_placeholderColor : m_textColor;
        text.origin = LFTextOrigin::Top;
        paint.drawText(text);
    }

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

            paint.drawLine(
                cursorX,
                cursorTop,
                cursorX,
                cursorBottom,
                m_cursorColor,
                1.2f
            );
        }
    }

    paint.popState();
}

void LFInput::onFocusChanged(bool focused) {
    if (focused) {
        setBorder(1.5f, 0xFF4C9AFF);
    } else {
        setBorder(1.0f, 0xFFDDDDDD);
    }
}
