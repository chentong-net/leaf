//
// EnglishWords app coordinator.
//

#ifndef ENGLISHWORDS_APP_H
#define ENGLISHWORDS_APP_H

#include "EnglishWordsDataManager.h"
#include "LFEngine.h"

#include <memory>

class LFAudioPlayer;

class EnglishWordsApp : public std::enable_shared_from_this<EnglishWordsApp> {
public:
    static std::shared_ptr<EnglishWordsApp> create();

    LFNode::Ptr start();

private:
    void showMainPage();
    void showLearningPage();
    void showTopicPage(const EnglishWordTopic& topic);
    void handleWordSelected(const EnglishWordEntry& entry);
    void playAudioPath(const std::string& path);
    void stopAudio();

    std::shared_ptr<LFNavigator> m_navigator;
    EnglishWordsDataManager::Ptr m_dataManager;
    std::shared_ptr<LFAudioPlayer> m_audioPlayer;
};

#endif // ENGLISHWORDS_APP_H
