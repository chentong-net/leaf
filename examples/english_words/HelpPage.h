//
// EnglishWords help page.
//

#ifndef ENGLISHWORDS_HELPPAGE_H
#define ENGLISHWORDS_HELPPAGE_H

#include "LFEngine.h"

class HelpPage : public LFPage {
public:
    static std::shared_ptr<HelpPage> create();

private:
    void buildUI();
};

#endif // ENGLISHWORDS_HELPPAGE_H
