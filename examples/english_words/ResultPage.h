//
// EnglishWords results page.
//

#ifndef ENGLISHWORDS_RESULTPAGE_H
#define ENGLISHWORDS_RESULTPAGE_H

#include "EnglishWordsDataManager.h"
#include "LFEngine.h"

#include <string>
#include <vector>

class ResultPage : public LFPage {
public:
    static std::shared_ptr<ResultPage> create();

private:
    void buildUI();
    void loadResults();
    void refreshList();
    void showStatus(const std::string& text);
    void openResult(const EnglishWordsSavedResultSummary& summary);

    std::string m_statusMessage;
    EnglishWordsDataManager::Ptr m_dataManager;
    std::vector<EnglishWordsSavedResultSummary> m_results;
    std::shared_ptr<LFListView> m_listView;
};

#endif // ENGLISHWORDS_RESULTPAGE_H
