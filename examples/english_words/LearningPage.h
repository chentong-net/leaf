//
// EnglishWords learning page.
//

#ifndef ENGLISHWORDS_LEARNINGPAGE_H
#define ENGLISHWORDS_LEARNINGPAGE_H

#include "EnglishWordsDataManager.h"
#include "LFEngine.h"

#include <string>
#include <vector>

class LearningPage : public LFPage {
public:
    static std::shared_ptr<LearningPage> create();

private:
    void buildUI();
    void loadLevels();
    void renderLevels();
    void showStatus(const std::string& text);

    EnglishWordsDataManager::Ptr m_dataManager;
    std::vector<EnglishWordLevel> m_levels;
    std::shared_ptr<LFLinear> m_content;
};

#endif // ENGLISHWORDS_LEARNINGPAGE_H
