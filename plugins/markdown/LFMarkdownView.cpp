#include "LFMarkdownView.h"

#include "LFMarkdownParser.h"
#include "LFMarkdownRenderer.h"
#include "view/layout/LFLinear.h"
#include "view/wrapped/LFScrollView.h"

#include <utility>

LFMarkdownView::Ptr LFMarkdownView::create() {
    return std::make_shared<LFMarkdownView>();
}

LFMarkdownView::LFMarkdownView() {
    matchParentWidth();
    matchParentHeight();

    m_scrollView = LFScrollView::createVertical();
    m_scrollView->matchParentWidth();
    m_scrollView->matchParentHeight();
    m_scrollView->setBackgroundColor(m_theme.backgroundColor);
    addChild(m_scrollView);
}

void LFMarkdownView::setMarkdown(const std::string& markdown) {
    if (m_markdown == markdown) return;
    m_markdown = markdown;
    rebuild();
}

void LFMarkdownView::setTheme(const LFMarkdownTheme& theme) {
    m_theme = theme;
    if (m_scrollView) {
        m_scrollView->setBackgroundColor(m_theme.backgroundColor);
    }
    rebuild();
}

void LFMarkdownView::setOnLinkTap(LinkCallback callback) {
    m_onLinkTap = std::move(callback);
    rebuild();
}

void LFMarkdownView::rebuild() {
    if (m_content) {
        m_content->removeFromParent();
        m_content.reset();
    }
    if (!m_scrollView) return;

    const LFMarkdownDocument document = LFMarkdownParser::parse(m_markdown);
    LFMarkdownRenderer renderer(m_theme, m_onLinkTap);
    m_content = renderer.render(document);
    m_scrollView->addChild(m_content);
    m_scrollView->scrollTo(0.0f, false);
}
