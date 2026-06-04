#include "LearningPage.h"

#include "EnglishWordsUI.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr float kLevelItemBottomSpacing = 12.0f;
constexpr float kLevelHeaderHeight = 74.0f;
constexpr float kLevelCardPadding = 10.0f;
constexpr float kLevelCardSpacing = 10.0f;
constexpr float kTopicItemExtent = 60.0f;
constexpr float kTopicRowHeight = 52.0f;

struct LevelStyle {
    uint32_t accentColor = 0xFF3567A3;
    uint32_t tintColor = 0xFFEAF3FF;
};

using TopicTapCallback = std::function<void(const EnglishWordTopic&)>;

LevelStyle resolveLevelStyle(const std::string& levelId, int index) {
    if (levelId == "1") return {0xFF3567A3, 0xFFEAF3FF};
    if (levelId == "2") return {0xFFAF7A1A, 0xFFFFF4DE};
    if (levelId == "3") return {0xFF2F8B61, 0xFFE9F7F0};
    if (levelId == "4") return {0xFFAF5A4A, 0xFFFFEEEA};

    static const std::vector<LevelStyle> kFallback = {
        {0xFF3567A3, 0xFFEAF3FF},
        {0xFFAF7A1A, 0xFFFFF4DE},
        {0xFF2F8B61, 0xFFE9F7F0},
        {0xFFAF5A4A, 0xFFFFEEEA},
    };
    return kFallback[static_cast<size_t>(index % static_cast<int>(kFallback.size()))];
}

class TopicListItemView : public LFLinear {
public:
    using Ptr = std::shared_ptr<TopicListItemView>;

    static Ptr create() {
        return std::make_shared<TopicListItemView>();
    }

    TopicListItemView() {
        matchParentWidth();
        wrapContentHeight();
        setPadding(YGEdgeBottom, kTopicItemExtent - kTopicRowHeight);

        m_row = LFLinear::createHorizontal();
        m_row->matchParentWidth();
        m_row->setHeight(kTopicRowHeight);
        m_row->setPadding(YGEdgeLeft, 14.0f);
        m_row->setPadding(YGEdgeRight, 14.0f);
        m_row->setBorderRadius(16.0f);
        m_row->setBorder(1.0f, 0xFFE0E8F2);
        m_row->setBackgroundColor(0xFFFFFFFF);
        m_row->setGravity(LFAlignment::Start, LFAlignment::Center);
        m_row->setDistribution(LFDistribution::SpaceBetween);
        m_row->setOnTap([this](const LFPoint&) {
            if (m_onTap) {
                m_onTap(m_topic);
            }
        });
        addChild(m_row);

        m_title = EnglishWordsUI::makeText("", 14.0f, 0xFF243244);
        m_title->setFlexGrow(1.0f);
        m_title->setFlexBasis(0.0f);
        m_title->setMaxLines(1);
        m_row->addChild(m_title);

        auto arrow = EnglishWordsUI::makeImage("EnglishWordsAssets/Images/icon-arrow-right.png", 14.0f);
        arrow->setOpacity(0.52f);
        m_row->addChild(arrow);
    }

    void bind(const EnglishWordTopic& topic, TopicTapCallback onTap) {
        m_topic = topic;
        m_onTap = std::move(onTap);
        m_title->setText(topic.title);
    }

private:
    EnglishWordTopic m_topic;
    TopicTapCallback m_onTap;
    std::shared_ptr<LFLinear> m_row;
    std::shared_ptr<LFText> m_title;
};

class TopicListAdapter : public LFListAdapter {
public:
    TopicListAdapter(const EnglishWordLevel& level, TopicTapCallback onTap)
        : m_level(level), m_onTap(std::move(onTap)) {
    }

    int getCount() override {
        return static_cast<int>(m_level.topics.size());
    }

    LFNode::Ptr createView() override {
        return TopicListItemView::create();
    }

    void bindView(LFNode::Ptr view, int index) override {
        if (!view || index < 0 || index >= static_cast<int>(m_level.topics.size())) {
            return;
        }
        auto item = std::static_pointer_cast<TopicListItemView>(view);
        item->bind(m_level.topics[static_cast<size_t>(index)], m_onTap);
    }

    float getItemExtent(int index) override {
        (void)index;
        return kTopicItemExtent;
    }

private:
    EnglishWordLevel m_level;
    TopicTapCallback m_onTap;
};

class LevelSectionView : public LFLinear {
public:
    using Ptr = std::shared_ptr<LevelSectionView>;

    static Ptr create() {
        return std::make_shared<LevelSectionView>();
    }

    LevelSectionView() {
        matchParentWidth();
        wrapContentHeight();
        setPadding(YGEdgeBottom, kLevelItemBottomSpacing);

        m_card = LFLinear::createVertical();
        m_card->matchParentWidth();
        m_card->wrapContentHeight();
        m_card->setPadding(YGEdgeAll, kLevelCardPadding);
        m_card->setSpacing(kLevelCardSpacing);
        m_card->setBorderRadius(22.0f);
        m_card->setBackgroundColor(0xFFFFFFFF);
        m_card->setShadow(0.0f, 10.0f, 24.0f, 0.0f, EnglishWordsUI::kCardShadowColor);
        addChild(m_card);

        m_headerRow = LFLinear::createHorizontal();
        m_headerRow->matchParentWidth();
        m_headerRow->setHeight(kLevelHeaderHeight);
        m_headerRow->setPadding(YGEdgeLeft, 16.0f);
        m_headerRow->setPadding(YGEdgeRight, 16.0f);
        m_headerRow->setBorderRadius(18.0f);
        m_headerRow->setGravity(LFAlignment::Start, LFAlignment::Center);
        m_headerRow->setDistribution(LFDistribution::SpaceBetween);
        m_headerRow->setOnTap([this](const LFPoint&) {
            if (m_onToggle) {
                m_onToggle();
            }
        });
        m_card->addChild(m_headerRow);

        m_title = EnglishWordsUI::makeText("", 17.0f, EnglishWordsUI::kTitleColor);
        m_title->setFlexGrow(1.0f);
        m_title->setFlexBasis(0.0f);
        m_title->setMaxLines(1);
        m_headerRow->addChild(m_title);

        m_iconBubble = LFBox::create();
        m_iconBubble->setWidth(40.0f);
        m_iconBubble->setHeight(40.0f);
        m_iconBubble->setBorderRadius(14.0f);
        m_iconBubble->setBorder(1.0f, 0x11E2E8F0);
        m_iconBubble->addChild(EnglishWordsUI::makeImage("EnglishWordsAssets/Images/icon-add.png", 18.0f), LFBoxAlign::Center);
        m_headerRow->addChild(m_iconBubble);

        m_topicList = LFListView::createVertical();
        m_topicList->matchParentWidth();
        m_topicList->setScrollBarEnabled(false);
        m_topicList->setBounces(false);
        m_topicList->setMasksToBounds(true);
        m_card->addChild(m_topicList);
    }

    void bind(const EnglishWordLevel& level,
              const LevelStyle& style,
              std::function<void()> onToggle,
              TopicTapCallback onTopicTap) {
        m_level = level;
        m_style = style;
        m_onToggle = std::move(onToggle);

        m_title->setText(level.title);
        m_topicList->setHeight(static_cast<float>(m_level.topics.size()) * kTopicItemExtent);
        m_topicList->setAdapter(std::make_shared<TopicListAdapter>(m_level, std::move(onTopicTap)));
        setExpanded(false);
    }

    void setExpanded(bool expanded) {
        if (expanded) {
            m_card->setBorder(1.5f, m_style.accentColor);
            m_headerRow->setBackgroundColor(m_style.tintColor);
            m_iconBubble->setBackgroundColor(m_style.accentColor);
            m_topicList->setDisplay(YGDisplayFlex);
        } else {
            m_card->setBorder(1.0f, EnglishWordsUI::kCardBorderColor);
            m_headerRow->setBackgroundColor(0xFFFFFFFF);
            m_iconBubble->setBackgroundColor(m_style.tintColor);
            m_topicList->setDisplay(YGDisplayNone);
        }
    }

private:
    EnglishWordLevel m_level;
    LevelStyle m_style;
    std::function<void()> m_onToggle;
    std::shared_ptr<LFLinear> m_card;
    std::shared_ptr<LFLinear> m_headerRow;
    std::shared_ptr<LFText> m_title;
    std::shared_ptr<LFBox> m_iconBubble;
    std::shared_ptr<LFListView> m_topicList;
};

} // namespace

std::shared_ptr<LearningPage> LearningPage::create(
    std::function<void()> onBack,
    std::function<void(const EnglishWordTopic&)> onTopicSelected) {
    auto page = std::make_shared<LearningPage>();
    page->m_onBack = std::move(onBack);
    page->m_onTopicSelected = std::move(onTopicSelected);
    page->setBackgroundColor(EnglishWordsUI::kPageBackgroundColor);
    page->buildUI();
    return page;
}

void LearningPage::buildUI() {
    auto root = EnglishWordsUI::createPageRoot();
    addChild(root);

    EnglishWordsUI::addPageHeader(root, "Study", [this]() {
        if (m_onBack) {
            m_onBack();
        }
    }, 22.0f);

    auto scrollView = LFScrollView::createVertical();
    scrollView->matchParentWidth();
    scrollView->setFlexGrow(1.0f);
    scrollView->setFlexBasis(0.0f);
    scrollView->setBounces(false);
    scrollView->setScrollBarEnabled(false);
    root->addChild(scrollView);

    m_content = LFLinear::createVertical();
    m_content->matchParentWidth();
    m_content->wrapContentHeight();
    m_content->setPadding(YGEdgeAll, 8.0f);
    m_content->setPadding(YGEdgeBottom, 20.0f);
    m_content->setSpacing(0.0f);
    m_content->setBackgroundColor(EnglishWordsUI::kSurfaceColor);
    m_content->setBorderRadius(28.0f);
    m_content->setBorder(1.0f, EnglishWordsUI::kSurfaceBorderColor);
    scrollView->addChild(m_content);
}

void LearningPage::setLevels(const std::vector<EnglishWordLevel>& levels) {
    m_levels = levels;
    renderLevels();
}

void LearningPage::showStatus(const std::string& text) {
    if (!m_content) {
        return;
    }
    EnglishWordsUI::clearChildren(m_content);
    m_content->addChild(EnglishWordsUI::makeStatusCard(text));
}

void LearningPage::renderLevels() {
    if (!m_content) {
        return;
    }
    if (m_levels.empty()) {
        showStatus("No levels available.");
        return;
    }

    EnglishWordsUI::clearChildren(m_content);

    auto expandedIndex = std::make_shared<int>(0);
    auto sections = std::make_shared<std::vector<LevelSectionView::Ptr>>();

    for (int index = 0; index < static_cast<int>(m_levels.size()); ++index) {
        auto section = LevelSectionView::create();
        section->bind(
            m_levels[static_cast<size_t>(index)],
            resolveLevelStyle(m_levels[static_cast<size_t>(index)].id, index),
            [expandedIndex, sections, index]() {
                *expandedIndex = (*expandedIndex == index) ? -1 : index;
                for (int sectionIndex = 0; sectionIndex < static_cast<int>(sections->size()); ++sectionIndex) {
                    if ((*sections)[static_cast<size_t>(sectionIndex)]) {
                        (*sections)[static_cast<size_t>(sectionIndex)]->setExpanded(sectionIndex == *expandedIndex);
                    }
                }
            },
            [this](const EnglishWordTopic& topic) {
                if (m_onTopicSelected) {
                    m_onTopicSelected(topic);
                }
            }
        );
        sections->push_back(section);
        m_content->addChild(section);
    }

    for (int index = 0; index < static_cast<int>(sections->size()); ++index) {
        (*sections)[static_cast<size_t>(index)]->setExpanded(index == *expandedIndex);
    }
}
