//
// EnglishWords home page.
//

#ifndef ENGLISHWORDS_MAINPAGE_H
#define ENGLISHWORDS_MAINPAGE_H

#include "LFEngine.h"

class MainPage : public LFPage {
public:
    static std::shared_ptr<MainPage> create();

private:
    void buildUI();
};

#endif // ENGLISHWORDS_MAINPAGE_H
