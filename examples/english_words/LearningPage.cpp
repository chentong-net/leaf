#include "LearningPage.h"

#include <functional>
#include <string>
#include <utility>

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

struct TopicSpec {
    const char* label;
};

struct LevelSpec {
    const char* title;
    uint32_t accentColor;
    uint32_t tintColor;
    const TopicSpec* topics;
    int topicCount;
};

using TopicTapCallback = std::function<void(const TopicSpec&)>;

std::shared_ptr<LFText> makeText(const std::string& text, float size, uint32_t color) {
    auto node = std::make_shared<LFText>();
    node->setText(text);
    node->setFontSize(size);
    node->setTextColor(color);
    node->setLineHeight(1.3f);
    return node;
}

std::shared_ptr<LFImage> makeImage(const std::string& src, float size) {
    auto image = std::make_shared<LFImage>();
    image->setWidth(size);
    image->setHeight(size);
    image->setFit(LFImageFit::Contain);
    image->setSrc(src);
    return image;
}

std::shared_ptr<LFLinear> makeIconButtonLikeSurface(const std::string& iconPath, std::function<void()> onTap) {
    auto surface = LFLinear::createHorizontal();
    surface->setWidth(46.0f);
    surface->setHeight(46.0f);
    surface->setBorderRadius(16.0f);
    surface->setBorder(1.0f, 0xFFD8E4F1);
    surface->setBackgroundColor(0xFFFFFFFF);
    surface->setShadow(0.0f, 6.0f, 18.0f, 0.0f, 0x10233B53);
    surface->setGravity(LFAlignment::Center, LFAlignment::Center);
    surface->addChild(makeImage(iconPath, 18.0f));
    surface->setOnTap([onTap = std::move(onTap)](const LFPoint&) {
        if (onTap) {
            onTap();
        }
    });
    return surface;
}

const TopicSpec kLevel1Topics[] = {
    {"1.01 My day"},
    {"1.02 Family"},
    {"1.03 Describing people"},
    {"1.04 Countries"},
};

const TopicSpec kLevel2Topics[] = {
    {"2.01 Everyday things"},
    {"2.02 Shopping"},
    {"2.03 Transport"},
    {"2.04 Internet"},
};

const TopicSpec kLevel3Topics[] = {
    {"3.01 People"},
    {"3.02 Business"},
    {"3.03 Air travel"},
    {"3.04 Environment"},
};

const TopicSpec kLevel4Topics[] = {
    {"4.01 School. College. University"},
    {"4.02 Air travel"},
    {"4.03 Success and Failure"},
    {"4.04 Adverbs"},
};

const LevelSpec kLevels[] = {
    {"1 - Beginner", 0xFF3567A3, 0xFFEAF3FF, kLevel1Topics, 4},
    {"2 - Elementary", 0xFFAF7A1A, 0xFFFFF4DE, kLevel2Topics, 4},
    {"3 - Intermediate", 0xFF2F8B61, 0xFFE9F7F0, kLevel3Topics, 4},
    {"4 - Advanced", 0xFFAF5A4A, 0xFFFFEEEA, kLevel4Topics, 4},
};

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

        m_title = makeText("", 14.0f, 0xFF243244);
        m_title->setFlexGrow(1.0f);
        m_title->setFlexBasis(0.0f);
        m_title->setMaxLines(1);
        m_row->addChild(m_title);

        auto arrow = makeImage("EnglishWordsAssets/Images/icon-arrow-right.png", 14.0f);
        arrow->setOpacity(0.52f);
        m_row->addChild(arrow);
    }

    void bind(const TopicSpec& topic, TopicTapCallback onTap) {
        m_topic = topic;
        m_onTap = std::move(onTap);
        m_title->setText(topic.label ? topic.label : "");
    }

private:
    TopicSpec m_topic{};
    TopicTapCallback m_onTap;
    std::shared_ptr<LFLinear> m_row;
    std::shared_ptr<LFText> m_title;
};

class TopicListAdapter : public LFListAdapter {
public:
    TopicListAdapter(const LevelSpec& level, TopicTapCallback onTap)
        : m_level(level), m_onTap(std::move(onTap)) {
    }

    int getCount() override {
        return m_level.topicCount;
    }

    LFNode::Ptr createView() override {
        return TopicListItemView::create();
    }

    void bindView(LFNode::Ptr view, int index) override {
        if (!view || index < 0 || index >= m_level.topicCount) return;
        auto item = std::static_pointer_cast<TopicListItemView>(view);
        item->bind(m_level.topics[index], m_onTap);
    }

    float getItemExtent(int index) override {
        (void)index;
        return kTopicItemExtent;
    }

private:
    LevelSpec m_level;
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

        m_title = makeText("", 17.0f, kTitleColor);
        m_title->setFlexGrow(1.0f);
        m_title->setFlexBasis(0.0f);
        m_title->setMaxLines(1);
        m_headerRow->addChild(m_title);

        m_iconBubble = LFBox::create();
        m_iconBubble->setWidth(40.0f);
        m_iconBubble->setHeight(40.0f);
        m_iconBubble->setBorderRadius(14.0f);
        m_iconBubble->setBorder(1.0f, 0x11E2E8F0);
        m_iconBubble->addChild(makeImage("EnglishWordsAssets/Images/icon-add.png", 18.0f), LFBoxAlign::Center);
        m_headerRow->addChild(m_iconBubble);

        m_topicList = LFListView::createVertical();
        m_topicList->matchParentWidth();
        m_topicList->setScrollBarEnabled(false);
        m_topicList->setBounces(false);
        m_topicList->setMasksToBounds(true);
        m_card->addChild(m_topicList);
    }

    void bind(const LevelSpec& level,
              std::function<void()> onToggle,
              TopicTapCallback onTopicTap) {
        m_level = level;
        m_onToggle = std::move(onToggle);

        m_title->setText(level.title ? level.title : "");
        m_topicList->setHeight(static_cast<float>(level.topicCount) * kTopicItemExtent);
        m_topicAdapter = std::make_shared<TopicListAdapter>(level, std::move(onTopicTap));
        m_topicList->setAdapter(m_topicAdapter);
        setExpanded(false);
    }

    void setExpanded(bool expanded) {
        m_expanded = expanded;

        if (expanded) {
            m_card->setBorder(1.5f, m_level.accentColor);
            m_headerRow->setBackgroundColor(m_level.tintColor);
            m_iconBubble->setBackgroundColor(m_level.accentColor);
            m_topicList->setDisplay(YGDisplayFlex);
        } else {
            m_card->setBorder(1.0f, kCardBorderColor);
            m_headerRow->setBackgroundColor(0xFFFFFFFF);
            m_iconBubble->setBackgroundColor(m_level.tintColor);
            m_topicList->setDisplay(YGDisplayNone);
        }
    }

private:
    LevelSpec m_level{};
    bool m_expanded = false;
    std::function<void()> m_onToggle;
    std::shared_ptr<LFLinear> m_card;
    std::shared_ptr<LFLinear> m_headerRow;
    std::shared_ptr<LFText> m_title;
    std::shared_ptr<LFBox> m_iconBubble;
    std::shared_ptr<LFListView> m_topicList;
    std::shared_ptr<TopicListAdapter> m_topicAdapter;
};

} // namespace

std::shared_ptr<LearningPage> LearningPage::create(std::weak_ptr<LFNavigator> nav) {
    auto page = std::make_shared<LearningPage>();
    page->setBackgroundColor(kPageBackgroundColor);
    page->initUI(nav);
    return page;
}

void LearningPage::initUI(std::weak_ptr<LFNavigator> nav) {
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

    auto backButton = makeIconButtonLikeSurface("EnglishWordsAssets/Images/icon-arrow-left.png", [nav]() {
        if (auto navigator = nav.lock()) {
            navigator->pop();
        }
    });
    headerRow->addChild(backButton);

    auto titleWrap = LFLinear::createVertical();
    titleWrap->setFlexGrow(1.0f);
    titleWrap->setFlexBasis(0.0f);
    titleWrap->wrapContentHeight();

    auto title = makeText("Study", 22.0f, kTitleColor);
    title->matchParentWidth();
    title->setTextHAlign(LFTextHAlign::Center);
    titleWrap->addChild(title);
    headerRow->addChild(titleWrap);

    auto spacer = LFBox::create();
    spacer->setWidth(46.0f);
    spacer->setHeight(46.0f);
    headerRow->addChild(spacer);

    root->addChild(headerRow);

    auto scrollView = LFScrollView::createVertical();
    scrollView->matchParentWidth();
    scrollView->setFlexGrow(1.0f);
    scrollView->setFlexBasis(0.0f);
    scrollView->setBounces(false);
    scrollView->setScrollBarEnabled(false);
    root->addChild(scrollView);

    auto content = LFLinear::createVertical();
    content->matchParentWidth();
    content->wrapContentHeight();
    content->setPadding(YGEdgeAll, 8.0f);
    content->setPadding(YGEdgeBottom, 20.0f);
    content->setSpacing(0.0f);
    content->setBackgroundColor(kSurfaceColor);
    content->setBorderRadius(28.0f);
    content->setBorder(1.0f, kSurfaceBorderColor);
    scrollView->addChild(content);

    auto expandedIndex = std::make_shared<int>(0);
    auto sections = std::make_shared<std::vector<LevelSectionView::Ptr>>();

    for (int i = 0; i < static_cast<int>(sizeof(kLevels) / sizeof(LevelSpec)); ++i) {
        auto section = LevelSectionView::create();
        section->bind(
            kLevels[i],
            [expandedIndex, sections, i]() {
                *expandedIndex = (*expandedIndex == i) ? -1 : i;
                for (int sectionIndex = 0; sectionIndex < static_cast<int>(sections->size()); ++sectionIndex) {
                    if ((*sections)[sectionIndex]) {
                        (*sections)[sectionIndex]->setExpanded(sectionIndex == *expandedIndex);
                    }
                }
            },
            [](const TopicSpec& topic) {
                (void)topic;
            }
        );
        sections->push_back(section);
        content->addChild(section);
    }

    for (int i = 0; i < static_cast<int>(sections->size()); ++i) {
        (*sections)[i]->setExpanded(i == *expandedIndex);
    }
}
