//
// EnglishWords learning page preview.
//

#ifndef ENGLISHWORDS_LEARNINGPAGE_H
#define ENGLISHWORDS_LEARNINGPAGE_H

#include "LFEngine.h"

class LearningPage : public LFPage {
public:
    static std::shared_ptr<LearningPage> create(std::weak_ptr<LFNavigator> nav);

private:
    void initUI(std::weak_ptr<LFNavigator> nav);
};

#endif // ENGLISHWORDS_LEARNINGPAGE_H
