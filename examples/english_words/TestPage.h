//
// EnglishWords test setup page.
//

#ifndef ENGLISHWORDS_TESTPAGE_H
#define ENGLISHWORDS_TESTPAGE_H

#include "EnglishWordsDataManager.h"
#include "LFEngine.h"

#include <vector>

class LFDropdown;

class TestPage : public LFPage {
public:
    static std::shared_ptr<TestPage> create();

private:
    void buildUI();
    void loadLevels();
    void renderLevels();
    void showStatus(const std::string& text);

    EnglishWordsDataManager::Ptr m_dataManager;
    std::vector<EnglishWordLevel> m_levels;
    EnglishWordsTestMode m_selectedMode = EnglishWordsTestMode::AudioToEnglish;
    std::shared_ptr<LFDropdown> m_modeDropdown;
    std::shared_ptr<LFLinear> m_content;
};

#endif // ENGLISHWORDS_TESTPAGE_H
