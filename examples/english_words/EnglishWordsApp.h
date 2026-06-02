//
// EnglishWords demo app entry.
//

#ifndef ENGLISHWORDS_APP_H
#define ENGLISHWORDS_APP_H

#include "LFEngine.h"

class EnglishWordsApp {
public:
    static std::shared_ptr<EnglishWordsApp> create();

    LFNode::Ptr start();

private:
    std::shared_ptr<LFNavigator> m_navigator;
};

#endif // ENGLISHWORDS_APP_H
