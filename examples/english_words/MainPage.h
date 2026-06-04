//
// EnglishWords home page.
//

#ifndef ENGLISHWORDS_MAINPAGE_H
#define ENGLISHWORDS_MAINPAGE_H

#include "LFEngine.h"

#include <functional>
#include <string>

class MainPage : public LFPage {
public:
    static std::shared_ptr<MainPage> create(std::function<void()> onStudyTap);

private:
    void buildUI();
    void updateStatus(const std::string& text);

    std::function<void()> m_onStudyTap;
    std::shared_ptr<LFText> m_statusText;
};

#endif // ENGLISHWORDS_MAINPAGE_H
