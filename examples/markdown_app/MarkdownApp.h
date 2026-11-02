//
// Markdown renderer showcase application.
//

#ifndef LEAF_MARKDOWNAPP_H
#define LEAF_MARKDOWNAPP_H

#include "LFEngine.h"

#include <string>

class MarkdownApp {
public:
    using Ptr = std::shared_ptr<MarkdownApp>;

    static Ptr create(const std::string& scenario = "");

    LFNode::Ptr start();

private:
    std::string m_scenario;
    std::shared_ptr<LFNavigator> m_navigator;
};

#endif // LEAF_MARKDOWNAPP_H
