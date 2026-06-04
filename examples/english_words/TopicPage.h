//
// EnglishWords topic page.
//

#ifndef ENGLISHWORDS_TOPICPAGE_H
#define ENGLISHWORDS_TOPICPAGE_H

#include "EnglishWordsDataManager.h"
#include "LFEngine.h"

#include <functional>
#include <string>
#include <vector>

class TopicPage : public LFPage {
public:
    static std::shared_ptr<TopicPage> create(
        const std::string& title,
        std::function<void()> onBack,
        std::function<void(const EnglishWordEntry&)> onEntrySelected);

    void setEntries(const std::vector<EnglishWordEntry>& entries);
    void showStatus(const std::string& text);

private:
    void buildUI();
    void refreshList();

    std::string m_title;
    std::string m_statusMessage = "Loading...";
    std::vector<EnglishWordEntry> m_entries;
    std::function<void()> m_onBack;
    std::function<void(const EnglishWordEntry&)> m_onEntrySelected;
    std::shared_ptr<LFListView> m_listView;
};

#endif // ENGLISHWORDS_TOPICPAGE_H
