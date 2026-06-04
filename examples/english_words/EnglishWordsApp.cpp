#include "EnglishWordsApp.h"

#include "LFAudioPlayer.h"
#include "LearningPage.h"
#include "MainPage.h"
#include "TopicPage.h"

#include <utility>

std::shared_ptr<EnglishWordsApp> EnglishWordsApp::create() {
    static auto app = std::make_shared<EnglishWordsApp>();
    return app;
}

LFNode::Ptr EnglishWordsApp::start() {
    m_navigator = LFNavigator::create();
    m_dataManager = EnglishWordsDataManager::create();
    showMainPage();
    return m_navigator;
}

void EnglishWordsApp::showMainPage() {
    if (!m_navigator) {
        return;
    }

    std::weak_ptr<EnglishWordsApp> weakSelf = shared_from_this();
    m_navigator->push(MainPage::create([weakSelf]() {
        if (auto self = weakSelf.lock()) {
            self->showLearningPage();
        }
    }), false);
}

void EnglishWordsApp::showLearningPage() {
    if (!m_navigator || !m_dataManager) {
        return;
    }

    stopAudio();

    std::weak_ptr<EnglishWordsApp> weakSelf = shared_from_this();
    auto page = LearningPage::create(
        [weakSelf]() {
            if (auto self = weakSelf.lock(); self && self->m_navigator) {
                self->stopAudio();
                self->m_navigator->pop();
            }
        },
        [weakSelf](const EnglishWordTopic& topic) {
            if (auto self = weakSelf.lock()) {
                self->showTopicPage(topic);
            }
        }
    );

    page->showStatus("Loading topics...");
    m_navigator->push(page);

    std::weak_ptr<LearningPage> weakPage = page;
    m_dataManager->loadLevels([weakPage](bool ok, std::vector<EnglishWordLevel> levels, const std::string&) {
        auto page = weakPage.lock();
        if (!page) {
            return;
        }

        if (ok) {
            page->setLevels(levels);
        } else {
            page->showStatus("Failed to load topics.");
        }
    });
}

void EnglishWordsApp::showTopicPage(const EnglishWordTopic& topic) {
    if (!m_navigator || !m_dataManager) {
        return;
    }

    stopAudio();

    std::weak_ptr<EnglishWordsApp> weakSelf = shared_from_this();
    auto page = TopicPage::create(
        topic.title,
        [weakSelf]() {
            if (auto self = weakSelf.lock(); self && self->m_navigator) {
                self->stopAudio();
                self->m_navigator->pop();
            }
        },
        [weakSelf](const EnglishWordEntry& entry) {
            if (auto self = weakSelf.lock()) {
                self->handleWordSelected(entry);
            }
        }
    );

    page->showStatus("Loading...");
    m_navigator->push(page);

    std::weak_ptr<TopicPage> weakPage = page;
    m_dataManager->loadEntries(topic, [weakPage](bool ok, std::vector<EnglishWordEntry> entries, const std::string&) {
        auto page = weakPage.lock();
        if (!page) {
            return;
        }

        if (ok) {
            page->setEntries(entries);
        } else {
            page->showStatus("Failed to load topic.");
        }
    });
}

void EnglishWordsApp::handleWordSelected(const EnglishWordEntry& entry) {
    if (entry.audioAssetPath.empty() || !m_dataManager) {
        return;
    }

    std::weak_ptr<EnglishWordsApp> weakSelf = shared_from_this();
    m_dataManager->resolveAudioPath(
        entry.audioAssetPath,
        [weakSelf](bool ok, std::string path, const std::string&) {
            if (auto self = weakSelf.lock(); self && ok) {
                self->playAudioPath(path);
            }
        }
    );
}

void EnglishWordsApp::playAudioPath(const std::string& path) {
    if (path.empty()) {
        return;
    }

    if (!m_audioPlayer) {
        m_audioPlayer = LFAudioPlayer::create();
        m_audioPlayer->setVolume(1.0f);
        m_audioPlayer->setOnError([](const LFAudioPlayerEvent&) {
        });
    }

    m_audioPlayer->stop();
    m_audioPlayer->setSource(path);
    m_audioPlayer->play();
}

void EnglishWordsApp::stopAudio() {
    if (m_audioPlayer) {
        m_audioPlayer->stop();
    }
}
