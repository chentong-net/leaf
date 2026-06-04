//
// EnglishWords home page.
//

#ifndef ENGLISHWORDS_MAINPAGE_H
#define ENGLISHWORDS_MAINPAGE_H

#include "LFEngine.h"

#include <string>

class MainPage : public LFPage {
public:
    static std::shared_ptr<MainPage> create();

private:
    void buildUI();
    void updateStatus(const std::string& text);

    std::shared_ptr<LFText> m_statusText;
};

#endif // ENGLISHWORDS_MAINPAGE_H
