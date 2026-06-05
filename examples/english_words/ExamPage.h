//
// EnglishWords exam page.
//

#ifndef ENGLISHWORDS_EXAMPAGE_H
#define ENGLISHWORDS_EXAMPAGE_H

#include "EnglishWordsDataManager.h"
#include "LFEngine.h"
#include "LFAudioPlayer.h"

#include <string>
#include <vector>

class LFButton;
class LFInput;

class ExamPage : public LFPage {
public:
    static std::shared_ptr<ExamPage> create(const EnglishWordTopic& topic, EnglishWordsTestMode mode);

    void onExit() override;

private:
    void buildUI();
    void loadEntries();
    void showStatus(const std::string& text);
    void refreshQuestion();
    void playCurrentAudio();
    void goToIndex(int index);
    void submitExam();

    EnglishWordTopic m_topic;
    EnglishWordsTestMode m_mode = EnglishWordsTestMode::AudioToEnglish;
    std::string m_statusMessage = "Loading...";
    EnglishWordsDataManager::Ptr m_dataManager;
    std::vector<EnglishWordEntry> m_entries;
    std::vector<std::string> m_userAnswers;
    int m_currentIndex = 0;
    std::shared_ptr<LFLinear> m_questionContainer;
    std::shared_ptr<LFText> m_progressText;
    std::shared_ptr<LFText> m_promptText;
    std::shared_ptr<LFButton> m_audioButton;
    std::shared_ptr<LFText> m_answerHintText;
    std::shared_ptr<LFInput> m_answerInput;
    std::shared_ptr<LFLinear> m_actionRow;
    std::shared_ptr<LFButton> m_previousButton;
    std::shared_ptr<LFButton> m_nextButton;
    std::shared_ptr<LFAudioPlayer> m_audioPlayer;
};

#endif // ENGLISHWORDS_EXAMPAGE_H
