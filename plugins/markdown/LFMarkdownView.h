//
// Cross-platform Markdown view composed from existing Leaf components.
//

#ifndef LEAF_LFMARKDOWNVIEW_H
#define LEAF_LFMARKDOWNVIEW_H

#include "view/layout/LFBox.h"

#include <string>

class LFMarkdownView : public LFBox {
public:
    using Ptr = std::shared_ptr<LFMarkdownView>;

    static Ptr create();

    LFMarkdownView();
    ~LFMarkdownView() override = default;

    void setMarkdown(const std::string& markdown);
    const std::string& getMarkdown() const { return m_markdown; }

private:
    std::string m_markdown;
};

#endif // LEAF_LFMARKDOWNVIEW_H
