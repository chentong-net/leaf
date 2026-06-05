#include "TopicPage.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr uint32_t kPageBackgroundColor = 0xFFF4F7FB;
constexpr uint32_t kTitleColor = 0xFF142033;
constexpr uint32_t kSubtitleColor = 0xFF6B7A90;
constexpr uint32_t kSurfaceColor = 0xFFEAF1F8;
constexpr uint32_t kSurfaceBorderColor = 0xFFDDE7F1;
constexpr uint32_t kCardBorderColor = 0xFFDCE6F2;
constexpr uint32_t kCardShadowColor = 0x12233B53;
constexpr uint32_t kHintColor = 0xFF3567A3;

constexpr float kWordItemExtent = 82.0f;
constexpr float kWordRowHeight = 74.0f;

std::shared_ptr<LFText> createText(const std::string& text, float size, uint32_t color) {
    auto node = std::make_shared<LFText>();
    node->setText(text);
    node->setFontSize(size);
    node->setTextColor(color);
    node->setLineHeight(1.3f);
    return node;
}

std::shared_ptr<LFImage> createImage(const std::string& src, float size) {
    auto image = std::make_shared<LFImage>();
    image->setWidth(size);
    image->setHeight(size);
    image->setFit(LFImageFit::Contain);
    image->setSrc(src);
    return image;
}

std::string entryTranslationText(const EnglishWordEntry& entry) {
    if (entry.russianTranslation.empty()) {
        return entry.chineseTranslation;
    }
    if (entry.chineseTranslation.empty()) {
        return entry.russianTranslation;
    }
    return entry.russianTranslation + "; " + entry.chineseTranslation;
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
        setPadding(YGEdgeLeft, 8.0f);
        setPadding(YGEdgeRight, 8.0f);
        setPadding(YGEdgeTop, 8.0f);

        m_row = LFLinear::createHorizontal();
        m_row->matchParentWidth();
        m_row->setHeight(kWordRowHeight);
        m_row->setPadding(YGEdgeLeft, 16.0f);
        m_row->setPadding(YGEdgeRight, 16.0f);
        m_row->setBorderRadius(18.0f);
        m_row->setBorder(1.0f, kCardBorderColor);
        m_row->setBackgroundColor(0xFFFFFFFF);
        m_row->setShadow(0.0f, 8.0f, 20.0f, 0.0f, kCardShadowColor);
        m_row->setGravity(LFAlignment::Start, LFAlignment::Center);
        m_row->setDistribution(LFDistribution::SpaceBetween);
        m_row->setOnTap([this](const LFPoint&) {
            if (m_onTap) {
                m_onTap(m_entry);
            }
        });
        addChild(m_row);

        m_copy = LFLinear::createVertical();
        m_copy->setFlexGrow(1.0f);
        m_copy->setFlexBasis(0.0f);
        m_copy->wrapContentHeight();
        m_copy->setSpacing(4.0f);
        m_row->addChild(m_copy);

        m_word = createText("", 16.0f, kTitleColor);
        m_word->matchParentWidth();
        m_word->setMaxLines(1);
        m_copy->addChild(m_word);

        m_translation = createText("", 13.0f, kSubtitleColor);
        m_translation->matchParentWidth();
        m_translation->setMaxLines(1);
        m_copy->addChild(m_translation);

        m_hint = createText("Play", 12.0f, kHintColor);
        m_hint->setMaxLines(1);
        m_row->addChild(m_hint);
    }

    void bindEntry(const EnglishWordEntry& entry, std::function<void(const EnglishWordEntry&)> onTap) {
        m_entry = entry;
        m_onTap = std::move(onTap);

        m_row->setBackgroundColor(0xFFFFFFFF);
        m_word->setText(entry.text);
        m_word->setTextHAlign(LFTextHAlign::Left);
        m_translation->setDisplay(YGDisplayFlex);
        m_translation->setText(entryTranslationText(entry));
        m_hint->setDisplay(entry.audioAssetPath.empty() ? YGDisplayNone : YGDisplayFlex);
    }

    void bindMessage(const std::string& text) {
        m_entry = EnglishWordEntry{};
        m_onTap = nullptr;

        m_row->setBackgroundColor(0xFFFFFFFF);
        m_word->setText(text);
        m_word->setTextHAlign(LFTextHAlign::Center);
        m_translation->setDisplay(YGDisplayNone);
        m_hint->setDisplay(YGDisplayNone);
    }

private:
    EnglishWordEntry m_entry;
    std::function<void(const EnglishWordEntry&)> m_onTap;
    std::shared_ptr<LFLinear> m_row;
    std::shared_ptr<LFLinear> m_copy;
    std::shared_ptr<LFText> m_word;
    std::shared_ptr<LFText> m_translation;
    std::shared_ptr<LFText> m_hint;
};

class TopicEntriesAdapter : public LFListAdapter {
public:
    TopicEntriesAdapter(std::function<const std::vector<EnglishWordEntry>*()> entriesProvider,
                        std::function<std::string()> statusProvider,
                        std::function<void(const EnglishWordEntry&)> onTap)
        : m_entriesProvider(std::move(entriesProvider)),
          m_statusProvider(std::move(statusProvider)),
          m_onTap(std::move(onTap)) {
    }

    int getCount() override {
        const auto* entries = m_entriesProvider ? m_entriesProvider() : nullptr;
        return (!entries || entries->empty()) ? 1 : static_cast<int>(entries->size());
    }

    LFNode::Ptr createView() override {
        return WordListItemView::create();
    }

    void bindView(LFNode::Ptr view, int index) override {
        auto item = std::static_pointer_cast<WordListItemView>(view);
        if (!item) {
            return;
        }

        const auto* entries = m_entriesProvider ? m_entriesProvider() : nullptr;
        if (!entries || entries->empty() || index < 0 || index >= static_cast<int>(entries->size())) {
            item->setMargin(YGEdgeBottom, 0.0f);
            item->bindMessage(m_statusProvider ? m_statusProvider() : "No words available.");
            return;
        }

        item->setMargin(YGEdgeBottom, index == static_cast<int>(entries->size()) - 1 ? 8.0f : 0.0f);
        item->bindEntry((*entries)[static_cast<size_t>(index)], m_onTap);
    }

    float getItemExtent(int index) override {
        (void)index;
        return kWordItemExtent;
    }

private:
    std::function<const std::vector<EnglishWordEntry>*()> m_entriesProvider;
    std::function<std::string()> m_statusProvider;
    std::function<void(const EnglishWordEntry&)> m_onTap;
};

} // namespace

std::shared_ptr<TopicPage> TopicPage::create(const EnglishWordTopic& topic) {
    auto page = std::make_shared<TopicPage>();
    page->m_topic = topic;
    page->m_dataManager = EnglishWordsDataManager::create();
    page->setBackgroundColor(kPageBackgroundColor);
    page->buildUI();
    page->loadEntries();
    return page;
}

void TopicPage::onExit() {
    if (m_audioPlayer) {
        m_audioPlayer->stop();
    }
    LFPage::onExit();
}

void TopicPage::buildUI() {
    auto root = LFLinear::createVertical();
    root->matchParentWidth();
    root->matchParentHeight();
    root->setPadding(YGEdgeTop, 20.0f);
    root->setPadding(YGEdgeLeft, 20.0f);
    root->setPadding(YGEdgeRight, 20.0f);
    root->setSpacing(16.0f);
    addChild(root);

    auto headerRow = LFLinear::createHorizontal();
    headerRow->matchParentWidth();
    headerRow->wrapContentHeight();
    headerRow->setAlignItems(YGAlignCenter);
    headerRow->setSpacing(12.0f);
    root->addChild(headerRow);

    auto backButton = LFLinear::createHorizontal();
    backButton->setWidth(46.0f);
    backButton->setHeight(46.0f);
    backButton->setBorderRadius(16.0f);
    backButton->setBorder(1.0f, 0xFFD8E4F1);
    backButton->setBackgroundColor(0xFFFFFFFF);
    backButton->setShadow(0.0f, 6.0f, 18.0f, 0.0f, 0x10233B53);
    backButton->setGravity(LFAlignment::Center, LFAlignment::Center);
    backButton->addChild(createImage("EnglishWordsAssets/Images/icon-arrow-left.png", 18.0f));
    headerRow->addChild(backButton);

    std::weak_ptr<TopicPage> weakSelf = std::static_pointer_cast<TopicPage>(shared_from_this());
    backButton->setOnTap([weakSelf](const LFPoint&) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        if (auto navigator = self->getNavigator()) {
            navigator->pop();
        }
    });

    auto titleWrap = LFLinear::createVertical();
    titleWrap->setFlexGrow(1.0f);
    titleWrap->setFlexBasis(0.0f);
    titleWrap->wrapContentHeight();
    headerRow->addChild(titleWrap);

    auto title = createText(m_topic.title, 22.0f, kTitleColor);
    title->matchParentWidth();
    title->setTextHAlign(LFTextHAlign::Center);
    title->setMaxLines(1);
    titleWrap->addChild(title);

    auto spacer = LFBox::create();
    spacer->setWidth(46.0f);
    spacer->setHeight(46.0f);
    headerRow->addChild(spacer);

    m_listView = LFListView::createVertical();
    m_listView->matchParentWidth();
    m_listView->setFlexGrow(1.0f);
    m_listView->setFlexBasis(0.0f);
    m_listView->setScrollBarEnabled(false);
    m_listView->setBounces(false);
    m_listView->setBackgroundColor(kSurfaceColor);
    m_listView->setBorderRadius(28.0f);
    m_listView->setBorder(1.0f, kSurfaceBorderColor);
    m_listView->setMargin(YGEdgeBottom, 20);
    root->addChild(m_listView);

    m_listView->setAdapter(std::make_shared<TopicEntriesAdapter>(
        [this]() -> const std::vector<EnglishWordEntry>* { return &m_entries; },
        [this]() { return m_statusMessage; },
        [this](const EnglishWordEntry& entry) {
            playEntryAudio(entry);
        }
    ));
}

void TopicPage::loadEntries() {
    showStatus("Loading words...");

    if (!m_dataManager) {
        showStatus("Failed to load words.");
        return;
    }

    std::weak_ptr<TopicPage> weakSelf = std::static_pointer_cast<TopicPage>(shared_from_this());
    m_dataManager->loadEntries(m_topic, [weakSelf](bool ok, std::vector<EnglishWordEntry> entries, const std::string&) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }

        if (!ok) {
            self->showStatus("Failed to load words.");
            return;
        }

        self->m_entries = std::move(entries);
        self->m_statusMessage = self->m_entries.empty() ? "No words available." : "";
        self->refreshList();
    });
}

void TopicPage::refreshList() {
    if (m_listView) {
        m_listView->notifyDataSetChanged();
    }
}

void TopicPage::showStatus(const std::string& text) {
    m_entries.clear();
    m_statusMessage = text;
    refreshList();
}

void TopicPage::playEntryAudio(const EnglishWordEntry& entry) {
    if (entry.audioAssetPath.empty() || !m_dataManager) {
        return;
    }

    if (!m_audioPlayer) {
        m_audioPlayer = LFAudioPlayer::create();
    }

    std::weak_ptr<TopicPage> weakSelf = std::static_pointer_cast<TopicPage>(shared_from_this());
    m_dataManager->resolveAudioPath(entry.audioAssetPath, [weakSelf](bool ok, std::string path, const std::string&) {
        auto self = weakSelf.lock();
        if (!self || !self->m_audioPlayer || !ok || path.empty()) {
            return;
        }

        self->m_audioPlayer->stop();
        self->m_audioPlayer->setSource(path);
        self->m_audioPlayer->play();
    });
}
