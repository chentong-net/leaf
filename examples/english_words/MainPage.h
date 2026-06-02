//
// EnglishWords home page preview.
//

#ifndef ENGLISHWORDS_MAINPAGE_H
#define ENGLISHWORDS_MAINPAGE_H

#include "LFEngine.h"

class MainPage {
public:
    static LFPage::Ptr create(std::weak_ptr<LFNavigator> nav);
};

#endif // ENGLISHWORDS_MAINPAGE_H
