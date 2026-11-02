//
// Platform-independent Markdown inline parser.
//

#ifndef LEAF_LFMARKDOWNINLINEPARSER_H
#define LEAF_LFMARKDOWNINLINEPARSER_H

#include "LFMarkdownDocument.h"

#include <string_view>
#include <vector>

class LFMarkdownInlineParser {
public:
    static std::vector<LFMarkdownInline> parse(std::string_view text);
};

#endif // LEAF_LFMARKDOWNINLINEPARSER_H
