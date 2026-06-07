//
// EnglishWords topic page.
//

#ifndef ENGLISHWORDS_TOPICPAGE_H
#define ENGLISHWORDS_TOPICPAGE_H

#include "EnglishWordsDataManager.h"
#include "LFEngine.h"
#include "LFAudioPlayer.h"

#include <string>
#include <vector>

class TopicPage : public LFPage {
public:
    static std::shared_ptr<TopicPage> create(const EnglishWordTopic& topic);

    void onExit() override;

private:
    void buildUI();
    void loadEntries();
    void refreshList();
    void showStatus(const std::string& text);
    void playEntryAudio(const EnglishWordEntry& entry);

    EnglishWordTopic m_topic;
    std::string m_statusMessage;
    EnglishWordsDataManager::Ptr m_dataManager;
    std::vector<EnglishWordEntry> m_entries;
    std::shared_ptr<LFListView> m_listView;
    std::shared_ptr<LFAudioPlayer> m_audioPlayer;
};

#endif // ENGLISHWORDS_TOPICPAGE_H
