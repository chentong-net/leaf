//
// EnglishWords score page.
//

#ifndef ENGLISHWORDS_POINTPAGE_H
#define ENGLISHWORDS_POINTPAGE_H

#include "EnglishWordsDataManager.h"
#include "LFEngine.h"

class PointPage : public LFPage {
public:
    static std::shared_ptr<PointPage> create(const EnglishWordsExamResult& result);

private:
    void buildUI();

    EnglishWordsExamResult m_result;
};

#endif // ENGLISHWORDS_POINTPAGE_H
