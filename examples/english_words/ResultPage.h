//
// EnglishWords results page.
//

#ifndef ENGLISHWORDS_RESULTPAGE_H
#define ENGLISHWORDS_RESULTPAGE_H

#include "EnglishWordsDataManager.h"
#include "LFEngine.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

class LFButton;
class LFCheckbox;
class LFLinear;
class LFText;

class ResultPage : public LFPage {
public:
    static std::shared_ptr<ResultPage> create();

private:
    void buildUI();
    void loadResults();
    void refreshList();
    void showStatus(const std::string& text);
    void openResult(const EnglishWordsSavedResultSummary& summary);
    void enterSelectionMode();
    void exitSelectionMode();
    void updateSelectionHeader();
    void updateDeleteButton();
    void toggleResultSelection(const EnglishWordsSavedResultSummary& summary);
    void setAllResultsSelected(bool selected);
    bool isResultSelected(const std::string& resultId) const;
    void confirmDeleteSelected();
    void deleteSelectedResults();

    std::string m_statusMessage;
    EnglishWordsDataManager::Ptr m_dataManager;
    std::vector<EnglishWordsSavedResultSummary> m_results;
    bool m_selectionMode = false;
    std::set<std::string> m_selectedResultIds;
    std::shared_ptr<LFListView> m_listView;
    std::shared_ptr<LFText> m_titleText;
    std::shared_ptr<LFLinear> m_leftSlot;
    std::shared_ptr<LFLinear> m_backButton;
    std::shared_ptr<LFButton> m_cancelButton;
    std::shared_ptr<LFButton> m_selectButton;
    std::shared_ptr<LFButton> m_selectAllButton;
    std::shared_ptr<LFButton> m_deleteButton;
};

#endif // ENGLISHWORDS_RESULTPAGE_H
