//
// Markdown renderer built from existing Leaf components.
//

#ifndef LEAF_LFMARKDOWNRENDERER_H
#define LEAF_LFMARKDOWNRENDERER_H

#include "LFMarkdownDocument.h"
#include "LFMarkdownView.h"
#include "view/layout/LFLinear.h"

class LFMarkdownRenderer {
public:
    LFMarkdownRenderer(const LFMarkdownTheme& theme,
                       const LFMarkdownView::LinkCallback& onLinkTap);

    std::shared_ptr<LFLinear> render(const LFMarkdownDocument& document) const;

private:
    void renderBlock(const LFMarkdownBlock& block,
                     const std::shared_ptr<LFLinear>& parent) const;
    void renderParagraph(const LFMarkdownBlock& block,
                         const std::shared_ptr<LFLinear>& parent,
                         float bottomSpacing) const;
    void renderHeading(const LFMarkdownBlock& block,
                       const std::shared_ptr<LFLinear>& parent) const;
    void renderQuote(const LFMarkdownBlock& block,
                     const std::shared_ptr<LFLinear>& parent) const;
    void renderList(const LFMarkdownBlock& block,
                    const std::shared_ptr<LFLinear>& parent,
                    float bottomSpacing,
                    bool nested) const;
    void renderCodeBlock(const LFMarkdownBlock& block,
                         const std::shared_ptr<LFLinear>& parent) const;
    void renderThematicBreak(const std::shared_ptr<LFLinear>& parent) const;

    std::string flattenInline(const std::vector<LFMarkdownInline>& inlines) const;

    const LFMarkdownTheme& m_theme;
    const LFMarkdownView::LinkCallback& m_onLinkTap;
};

#endif // LEAF_LFMARKDOWNRENDERER_H
