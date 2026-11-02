//
// Markdown renderer showcase application.
//

#ifndef LEAF_MARKDOWNAPP_H
#define LEAF_MARKDOWNAPP_H

#include "LFEngine.h"

class MarkdownApp {
public:
    using Ptr = std::shared_ptr<MarkdownApp>;

    static Ptr create();

    LFNode::Ptr start();

private:
    std::shared_ptr<LFNavigator> m_navigator;
};

#endif // LEAF_MARKDOWNAPP_H
