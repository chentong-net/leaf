//
// EnglishWords learning page.
//

#ifndef ENGLISHWORDS_LEARNINGPAGE_H
#define ENGLISHWORDS_LEARNINGPAGE_H

#include "EnglishWordsDataManager.h"
#include "LFEngine.h"

#include <functional>
#include <string>
#include <vector>

class LearningPage : public LFPage {
public:
    static std::shared_ptr<LearningPage> create(
        std::function<void()> onBack,
        std::function<void(const EnglishWordTopic&)> onTopicSelected);

    void setLevels(const std::vector<EnglishWordLevel>& levels);
    void showStatus(const std::string& text);

private:
    void buildUI();
    void renderLevels();

    std::function<void()> m_onBack;
    std::function<void(const EnglishWordTopic&)> m_onTopicSelected;
    std::vector<EnglishWordLevel> m_levels;
    std::shared_ptr<LFLinear> m_content;
};

#endif // ENGLISHWORDS_LEARNINGPAGE_H
