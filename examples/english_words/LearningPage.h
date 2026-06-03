//
// EnglishWords learning page preview.
//

#ifndef ENGLISHWORDS_LEARNINGPAGE_H
#define ENGLISHWORDS_LEARNINGPAGE_H

#include "EnglishWordsData.h"
#include "LFEngine.h"

class LearningPage : public LFPage {
public:
    static std::shared_ptr<LearningPage> create(std::weak_ptr<LFNavigator> nav);

private:
    void initUI(std::weak_ptr<LFNavigator> nav);
    void loadLevels(std::weak_ptr<LFNavigator> nav);
    void renderLevels(const std::vector<EnglishWordLevel>& levels, std::weak_ptr<LFNavigator> nav);
    void showStatus(const std::string& text);

    std::shared_ptr<LFLinear> m_content;
};

#endif // ENGLISHWORDS_LEARNINGPAGE_H
