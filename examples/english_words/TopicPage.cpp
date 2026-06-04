#include "TopicPage.h"

#include "EnglishWordsUI.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr uint32_t kHintColor = 0xFF3567A3;

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

    void bindEntry(const EnglishWordEntry& entry, std::function<void(const EnglishWordEntry&)> onTap) {
        m_entry = entry;
        m_onTap = std::move(onTap);

        m_card->setBackgroundColor(0xFFFFFFFF);
        m_word->setText(entry.text);
        m_word->setTextHAlign(LFTextHAlign::Left);
        m_translation->setDisplay(YGDisplayFlex);
        m_translation->setText(entry.translation);
        m_hint->setDisplay(entry.audioAssetPath.empty() ? YGDisplayNone : YGDisplayFlex);
    }

    void bindMessage(const std::string& text) {
        m_entry = EnglishWordEntry{};
        m_onTap = nullptr;

        m_card->setBackgroundColor(0xFFEAF1F8);
        m_word->setText(text);
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
            item->bindMessage(m_statusProvider ? m_statusProvider() : "No words available.");
            return;
        }

        item->bindEntry((*entries)[static_cast<size_t>(index)], m_onTap);
    }

    float getEstimatedItemExtent(int index) override {
        (void)index;
        const auto* entries = m_entriesProvider ? m_entriesProvider() : nullptr;
        return (!entries || entries->empty()) ? 88.0f : 94.0f;
    }

private:
    std::function<const std::vector<EnglishWordEntry>*()> m_entriesProvider;
    std::function<std::string()> m_statusProvider;
    std::function<void(const EnglishWordEntry&)> m_onTap;
};

} // namespace

std::shared_ptr<TopicPage> TopicPage::create(
    const std::string& title,
    std::function<void()> onBack,
    std::function<void(const EnglishWordEntry&)> onEntrySelected) {
    auto page = std::make_shared<TopicPage>();
    page->m_title = title;
    page->m_onBack = std::move(onBack);
    page->m_onEntrySelected = std::move(onEntrySelected);
    page->setBackgroundColor(EnglishWordsUI::kPageBackgroundColor);
    page->buildUI();
    return page;
}

void TopicPage::buildUI() {
    auto root = EnglishWordsUI::createPageRoot(20.0f);
    addChild(root);

    EnglishWordsUI::addPageHeader(root, m_title, [this]() {
        if (m_onBack) {
            m_onBack();
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

    m_listView->setAdapter(std::make_shared<TopicEntriesAdapter>(
        [this]() -> const std::vector<EnglishWordEntry>* { return &m_entries; },
        [this]() { return m_statusMessage; },
        [this](const EnglishWordEntry& entry) {
            if (m_onEntrySelected) {
                m_onEntrySelected(entry);
            }
        }
    ));
}

void TopicPage::setEntries(const std::vector<EnglishWordEntry>& entries) {
    m_entries = entries;
    m_statusMessage = m_entries.empty() ? "No words available." : "";
    refreshList();
}

void TopicPage::showStatus(const std::string& text) {
    m_entries.clear();
    m_statusMessage = text;
    refreshList();
}

void TopicPage::refreshList() {
    if (m_listView) {
        m_listView->notifyDataSetChanged();
    }
}
