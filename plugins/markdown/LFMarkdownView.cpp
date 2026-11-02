#include "LFMarkdownView.h"

LFMarkdownView::Ptr LFMarkdownView::create() {
    return std::make_shared<LFMarkdownView>();
}

LFMarkdownView::LFMarkdownView() {
    matchParentWidth();
    matchParentHeight();
}

void LFMarkdownView::setMarkdown(const std::string& markdown) {
    if (m_markdown == markdown) return;
    m_markdown = markdown;
    markDirty();
}
