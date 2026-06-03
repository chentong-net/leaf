//
// EnglishWords topic details page.
//

#ifndef ENGLISHWORDS_TOPICPAGE_H
#define ENGLISHWORDS_TOPICPAGE_H

#include "EnglishWordsData.h"
#include "LFEngine.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class LFAudioPlayer;

struct TopicPageState;

class TopicPage : public LFPage {
public:
    static std::shared_ptr<TopicPage> create(std::weak_ptr<LFNavigator> nav, EnglishWordTopic topic);

    void onExit() override;

private:
    void initUI(std::weak_ptr<LFNavigator> nav);
    void loadEntries();
    void refreshList();
    void playEntryAudio(const EnglishWordEntry& entry);
    void resolveTemporaryDirectory(std::function<void(const std::string&)> callback);
    void playAudioFile(const std::string& filePath);

    EnglishWordTopic m_topic;
    std::shared_ptr<LFListView> m_listView;
    std::shared_ptr<TopicPageState> m_state;
};

#endif // ENGLISHWORDS_TOPICPAGE_H
