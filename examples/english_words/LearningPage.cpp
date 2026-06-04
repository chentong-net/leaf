#include "LearningPage.h"

#include "TopicPage.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr uint32_t kPageBackgroundColor = 0xFFF4F7FB;
constexpr uint32_t kTitleColor = 0xFF142033;
constexpr uint32_t kSurfaceColor = 0xFFEAF1F8;
constexpr uint32_t kSurfaceBorderColor = 0xFFDDE7F1;
constexpr uint32_t kCardBorderColor = 0xFFDCE6F2;
constexpr uint32_t kCardShadowColor = 0x12233B53;

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

void clearChildren(const LFNode::Ptr& node) {
    if (!node) {
        return;
    }

    auto children = node->getChildren();
    for (const auto& child : children) {
        node->removeChild(child);
    }
}

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

        m_title = createText("", 14.0f, 0xFF243244);
        m_title->setFlexGrow(1.0f);
        m_title->setFlexBasis(0.0f);
        m_title->setMaxLines(1);
        m_row->addChild(m_title);

        auto arrow = createImage("EnglishWordsAssets/Images/icon-arrow-right.png", 14.0f);
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
        m_card->setShadow(0.0f, 10.0f, 24.0f, 0.0f, kCardShadowColor);
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

        m_title = createText("", 17.0f, kTitleColor);
        m_title->setFlexGrow(1.0f);
        m_title->setFlexBasis(0.0f);
        m_title->setMaxLines(1);
        m_headerRow->addChild(m_title);

        m_iconBubble = LFBox::create();
        m_iconBubble->setWidth(40.0f);
        m_iconBubble->setHeight(40.0f);
        m_iconBubble->setBorderRadius(14.0f);
        m_iconBubble->setBorder(1.0f, 0x11E2E8F0);
        m_iconBubble->addChild(createImage("EnglishWordsAssets/Images/icon-add.png", 18.0f), LFBoxAlign::Center);
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
        m_style = style;
        m_onToggle = std::move(onToggle);

        m_title->setText(level.title);
        m_topicList->setHeight(static_cast<float>(level.topics.size()) * kTopicItemExtent);
        m_topicList->setAdapter(std::make_shared<TopicListAdapter>(level, std::move(onTopicTap)));
        setExpanded(false);
    }

    void setExpanded(bool expanded) {
        if (expanded) {
            m_card->setBorder(1.5f, m_style.accentColor);
            m_headerRow->setBackgroundColor(m_style.tintColor);
            m_iconBubble->setBackgroundColor(m_style.accentColor);
            m_topicList->setDisplay(YGDisplayFlex);
        } else {
            m_card->setBorder(1.0f, kCardBorderColor);
            m_headerRow->setBackgroundColor(0xFFFFFFFF);
            m_iconBubble->setBackgroundColor(m_style.tintColor);
            m_topicList->setDisplay(YGDisplayNone);
        }
    }

private:
    LevelStyle m_style;
    std::function<void()> m_onToggle;
    std::shared_ptr<LFLinear> m_card;
    std::shared_ptr<LFLinear> m_headerRow;
    std::shared_ptr<LFText> m_title;
    std::shared_ptr<LFBox> m_iconBubble;
    std::shared_ptr<LFListView> m_topicList;
};

} // namespace

std::shared_ptr<LearningPage> LearningPage::create() {
    auto page = std::make_shared<LearningPage>();
    page->m_dataManager = EnglishWordsDataManager::create();
    page->setBackgroundColor(kPageBackgroundColor);
    page->buildUI();
    page->loadLevels();
    return page;
}

void LearningPage::buildUI() {
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

    std::weak_ptr<LearningPage> weakSelf = std::static_pointer_cast<LearningPage>(shared_from_this());
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

    auto title = createText("Study", 22.0f, kTitleColor);
    title->matchParentWidth();
    title->setTextHAlign(LFTextHAlign::Center);
    titleWrap->addChild(title);

    auto spacer = LFBox::create();
    spacer->setWidth(46.0f);
    spacer->setHeight(46.0f);
    headerRow->addChild(spacer);

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
    m_content->setBackgroundColor(kSurfaceColor);
    m_content->setBorderRadius(28.0f);
    m_content->setBorder(1.0f, kSurfaceBorderColor);
    scrollView->addChild(m_content);
}

void LearningPage::loadLevels() {
    showStatus("Loading topics...");

    if (!m_dataManager) {
        showStatus("Failed to load topics.");
        return;
    }

    std::weak_ptr<LearningPage> weakSelf = std::static_pointer_cast<LearningPage>(shared_from_this());
    m_dataManager->loadLevels([weakSelf](bool ok, std::vector<EnglishWordLevel> levels, const std::string&) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }

        if (!ok) {
            self->showStatus("Failed to load topics.");
            return;
        }

        self->m_levels = std::move(levels);
        self->renderLevels();
    });
}

void LearningPage::renderLevels() {
    if (!m_content) {
        return;
    }
    if (m_levels.empty()) {
        showStatus("No levels available.");
        return;
    }

    clearChildren(m_content);

    auto expandedIndex = std::make_shared<int>(0);
    auto sections = std::make_shared<std::vector<LevelSectionView::Ptr>>();
    std::weak_ptr<LearningPage> weakSelf = std::static_pointer_cast<LearningPage>(shared_from_this());

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
            [weakSelf](const EnglishWordTopic& topic) {
                auto self = weakSelf.lock();
                if (!self) {
                    return;
                }
                if (auto navigator = self->getNavigator()) {
                    navigator->push(TopicPage::create(topic));
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

void LearningPage::showStatus(const std::string& text) {
    if (!m_content) {
        return;
    }

    clearChildren(m_content);

    auto card = LFLinear::createVertical();
    card->matchParentWidth();
    card->wrapContentHeight();
    card->setPadding(YGEdgeAll, 24.0f);
    card->setBorderRadius(22.0f);
    card->setBackgroundColor(0xFFFFFFFF);
    card->setBorder(1.0f, kCardBorderColor);
    card->setShadow(0.0f, 8.0f, 22.0f, 0.0f, kCardShadowColor);
    m_content->addChild(card);

    auto message = createText(text, 14.0f, 0xFF526275);
    message->matchParentWidth();
    message->setTextHAlign(LFTextHAlign::Center);
    card->addChild(message);
}
