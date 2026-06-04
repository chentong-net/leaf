#include "TopicPage.h"

#include "EnglishWordsUI.h"
#include "LFAudioPlayer.h"
#include "LFPathProvider.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <utility>

struct TopicPageState {
    std::vector<EnglishWordEntry> entries;
    std::string statusMessage = "Loading...";
    std::string tempDirectory;
    std::unordered_map<std::string, std::string> cachedAudioFiles;
    std::shared_ptr<LFAudioPlayer> audioPlayer;
};

namespace {

constexpr uint32_t kHintColor = 0xFF3567A3;

bool writeBinaryFile(const std::string& path, const std::shared_ptr<LFData>& data) {
    if (path.empty() || !data || !data->data || data->size == 0) {
        return false;
    }

    const auto filePath = std::filesystem::u8path(path);
    const auto parent = filePath.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return false;
        }
    }

    std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output.write(reinterpret_cast<const char*>(data->data), static_cast<std::streamsize>(data->size));
    return output.good();
}

std::string fallbackTemporaryDirectory() {
#if defined(__DESKTOP__)
    std::error_code ec;
    const auto path = std::filesystem::temp_directory_path(ec);
    if (!ec) {
        return path.u8string();
    }
#endif
    return "";
}

std::string buildCachedAudioPath(const std::string& directory, const std::string& assetPath) {
    const size_t key = std::hash<std::string>{}(assetPath);
    const auto path = std::filesystem::u8path(directory) /
                      ("leaf_english_words_" + std::to_string(static_cast<unsigned long long>(key)) + ".mp3");
    return path.u8string();
}

class WordListItemView : public LFLinear {
public:
    using Ptr = std::shared_ptr<WordListItemView>;

    static Ptr create() {
        return std::make_shared<WordListItemView>();
    }

    WordListItemView() {
        matchParentWidth();
        wrapContentHeight();
        setPadding(YGEdgeBottom, 12.0f);

        m_card = LFLinear::createHorizontal();
        m_card->matchParentWidth();
        m_card->wrapContentHeight();
        m_card->setPadding(YGEdgeAll, 16.0f);
        m_card->setSpacing(14.0f);
        m_card->setBorderRadius(18.0f);
        m_card->setBorder(1.0f, EnglishWordsUI::kCardBorderColor);
        m_card->setBackgroundColor(0xFFFFFFFF);
        m_card->setShadow(0.0f, 8.0f, 20.0f, 0.0f, EnglishWordsUI::kCardShadowColor);
        m_card->setGravity(LFAlignment::Start, LFAlignment::Center);
        m_card->setOnTap([this](const LFPoint&) {
            if (m_onTap) {
                m_onTap(m_entry);
            }
        });
        addChild(m_card);

        m_copy = LFLinear::createVertical();
        m_copy->setFlexGrow(1.0f);
        m_copy->setFlexBasis(0.0f);
        m_copy->wrapContentHeight();
        m_copy->setSpacing(4.0f);
        m_card->addChild(m_copy);

        m_word = EnglishWordsUI::makeText("", 16.0f, EnglishWordsUI::kTitleColor);
        m_word->matchParentWidth();
        m_copy->addChild(m_word);

        m_translation = EnglishWordsUI::makeText("", 13.0f, 0xFF657489);
        m_translation->matchParentWidth();
        m_copy->addChild(m_translation);

        m_hint = EnglishWordsUI::makeText("Play", 12.0f, kHintColor);
        m_hint->setMaxLines(1);
        m_card->addChild(m_hint);
    }

    void bindWord(const EnglishWordEntry& entry, std::function<void(const EnglishWordEntry&)> onTap) {
        m_entry = entry;
        m_onTap = std::move(onTap);

        m_card->setBackgroundColor(0xFFFFFFFF);
        m_word->setText(entry.text);
        m_word->setTextHAlign(LFTextHAlign::Left);
        m_translation->setDisplay(YGDisplayFlex);
        m_translation->setText(entry.translation);

        if (!entry.audioAssetPath.empty()) {
            m_hint->setDisplay(YGDisplayFlex);
            m_hint->setText("Play");
        } else {
            m_hint->setDisplay(YGDisplayNone);
        }
    }

    void bindMessage(const std::string& message) {
        m_entry = EnglishWordEntry{};
        m_onTap = nullptr;

        m_card->setBackgroundColor(0xFFEAF1F8);
        m_word->setText(message);
        m_word->setTextHAlign(LFTextHAlign::Center);
        m_translation->setDisplay(YGDisplayNone);
        m_hint->setDisplay(YGDisplayNone);
    }

private:
    EnglishWordEntry m_entry;
    std::function<void(const EnglishWordEntry&)> m_onTap;
    std::shared_ptr<LFLinear> m_card;
    std::shared_ptr<LFLinear> m_copy;
    std::shared_ptr<LFText> m_word;
    std::shared_ptr<LFText> m_translation;
    std::shared_ptr<LFText> m_hint;
};

class TopicEntriesAdapter : public LFListAdapter {
public:
    TopicEntriesAdapter(std::shared_ptr<TopicPageState> state,
                        std::function<void(const EnglishWordEntry&)> onTap)
        : m_state(std::move(state)), m_onTap(std::move(onTap)) {
    }

    int getCount() override {
        if (!m_state) {
            return 1;
        }
        return m_state->entries.empty() ? 1 : static_cast<int>(m_state->entries.size());
    }

    LFNode::Ptr createView() override {
        return WordListItemView::create();
    }

    void bindView(LFNode::Ptr view, int index) override {
        auto item = std::static_pointer_cast<WordListItemView>(view);
        if (!item || !m_state) {
            return;
        }

        if (m_state->entries.empty()) {
            item->bindMessage(m_state->statusMessage.empty() ? "No words available." : m_state->statusMessage);
            return;
        }

        if (index < 0 || index >= static_cast<int>(m_state->entries.size())) {
            item->bindMessage("No words available.");
            return;
        }

        item->bindWord(m_state->entries[static_cast<size_t>(index)], m_onTap);
    }

    float getEstimatedItemExtent(int index) override {
        (void)index;
        return m_state && m_state->entries.empty() ? 88.0f : 94.0f;
    }

private:
    std::shared_ptr<TopicPageState> m_state;
    std::function<void(const EnglishWordEntry&)> m_onTap;
};

} // namespace

std::shared_ptr<TopicPage> TopicPage::create(std::weak_ptr<LFNavigator> nav, EnglishWordTopic topic) {
    auto page = std::make_shared<TopicPage>();
    page->m_topic = std::move(topic);
    page->setBackgroundColor(EnglishWordsUI::kPageBackgroundColor);
    page->initUI(nav);
    return page;
}

void TopicPage::onExit() {
    if (m_state && m_state->audioPlayer) {
        m_state->audioPlayer->stop();
    }
}

void TopicPage::initUI(std::weak_ptr<LFNavigator> nav) {
    m_state = std::make_shared<TopicPageState>();

    auto root = EnglishWordsUI::createPageRoot(20.0f);
    addChild(root);

    EnglishWordsUI::addPageHeader(root, m_topic.title, [nav]() {
        if (auto navigator = nav.lock()) {
            navigator->pop();
        }
    }, 20.0f);

    m_listView = LFListView::createVertical();
    m_listView->matchParentWidth();
    m_listView->setFlexGrow(1.0f);
    m_listView->setFlexBasis(0.0f);
    m_listView->setScrollBarEnabled(false);
    m_listView->setBounces(false);
    m_listView->setBackgroundColor(EnglishWordsUI::kSurfaceColor);
    m_listView->setBorder(1.0f, EnglishWordsUI::kSurfaceBorderColor);
    m_listView->setBorderRadius(28.0f);
    root->addChild(m_listView);

    std::weak_ptr<TopicPage> weakSelf = std::static_pointer_cast<TopicPage>(shared_from_this());
    auto adapter = std::make_shared<TopicEntriesAdapter>(
        m_state,
        [weakSelf](const EnglishWordEntry& entry) {
            if (auto self = weakSelf.lock()) {
                self->playEntryAudio(entry);
            }
        }
    );
    m_listView->setAdapter(adapter);

    loadEntries();
}

void TopicPage::loadEntries() {
    std::weak_ptr<TopicPage> weakSelf = std::static_pointer_cast<TopicPage>(shared_from_this());
    loadEnglishWordEntries(
        m_topic,
        [weakSelf](bool ok, std::vector<EnglishWordEntry> entries, const std::string&) {
            auto self = weakSelf.lock();
            if (!self || !self->m_state) {
                return;
            }

            if (ok) {
                self->m_state->entries = std::move(entries);
                self->m_state->statusMessage = self->m_state->entries.empty()
                    ? "No words available."
                    : "";
            } else {
                self->m_state->entries.clear();
                self->m_state->statusMessage = "Failed to load topic.";
            }

            self->refreshList();
        }
    );
}

void TopicPage::refreshList() {
    if (m_listView) {
        m_listView->notifyDataSetChanged();
    }
}

void TopicPage::playEntryAudio(const EnglishWordEntry& entry) {
    if (entry.audioAssetPath.empty() || !m_state) {
        return;
    }

    std::weak_ptr<TopicPage> weakSelf = std::static_pointer_cast<TopicPage>(shared_from_this());
    resolveTemporaryDirectory(
        [weakSelf, entry](const std::string& directory) {
            auto self = weakSelf.lock();
            if (!self || !self->m_state || directory.empty()) {
                return;
            }

            auto cached = self->m_state->cachedAudioFiles.find(entry.audioAssetPath);
            if (cached != self->m_state->cachedAudioFiles.end()) {
                std::error_code ec;
                if (std::filesystem::exists(std::filesystem::u8path(cached->second), ec) && !ec) {
                    self->playAudioFile(cached->second);
                    return;
                }
            }

            LFResourceProvider::getInstance().fetchAsset(
                entry.audioAssetPath,
                [weakSelf, entry, directory](std::shared_ptr<LFData> data) {
                    auto self = weakSelf.lock();
                    if (!self || !self->m_state || !data || !data->data || data->size == 0) {
                        return;
                    }

                    const std::string filePath = buildCachedAudioPath(directory, entry.audioAssetPath);
                    if (!writeBinaryFile(filePath, data)) {
                        return;
                    }

                    self->m_state->cachedAudioFiles[entry.audioAssetPath] = filePath;
                    self->playAudioFile(filePath);
                }
            );
        }
    );
}

void TopicPage::resolveTemporaryDirectory(std::function<void(const std::string&)> callback) {
    if (!callback || !m_state) {
        return;
    }

    if (!m_state->tempDirectory.empty()) {
        callback(m_state->tempDirectory);
        return;
    }

    const std::string fallback = fallbackTemporaryDirectory();
    if (!fallback.empty()) {
        m_state->tempDirectory = fallback;
        callback(m_state->tempDirectory);
        return;
    }

    std::weak_ptr<TopicPage> weakSelf = std::static_pointer_cast<TopicPage>(shared_from_this());
    LFPathProvider::getTemporaryPath([weakSelf, callback = std::move(callback)](const LFPathProviderResult& result) mutable {
        auto self = weakSelf.lock();
        if (!self || !self->m_state) {
            return;
        }

        self->m_state->tempDirectory = result.ok ? result.path : "";
        callback(self->m_state->tempDirectory);
    });
}

void TopicPage::playAudioFile(const std::string& filePath) {
    if (filePath.empty() || !m_state) {
        return;
    }

    if (!m_state->audioPlayer) {
        m_state->audioPlayer = LFAudioPlayer::create();
        m_state->audioPlayer->setVolume(1.0f);
        m_state->audioPlayer->setOnError([](const LFAudioPlayerEvent&) {
        });
    }

    m_state->audioPlayer->stop();
    m_state->audioPlayer->setSource(filePath);
    m_state->audioPlayer->play();
}
