//
// Platform-independent Markdown parser.
//

#ifndef LEAF_LFMARKDOWNPARSER_H
#define LEAF_LFMARKDOWNPARSER_H

#include "LFMarkdownDocument.h"

#include <string>

class LFMarkdownParser {
public:
    static LFMarkdownDocument parse(const std::string& markdown);
};

#endif // LEAF_LFMARKDOWNPARSER_H
