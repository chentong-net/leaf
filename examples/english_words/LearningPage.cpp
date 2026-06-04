#include "LearningPage.h"

#include "TopicPage.h"

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr uint32_t kPageBackgroundColor = 0xFFF4F7FB;
constexpr uint32_t kTitleColor = 0xFF142033;
constexpr uint32_t kMetaColor = 0xFF5F7086;
constexpr uint32_t kSurfaceColor = 0xFFEAF1F8;
constexpr uint32_t kSurfaceBorderColor = 0xFFDDE7F1;
constexpr uint32_t kCardBorderColor = 0xFFDCE6F2;
constexpr uint32_t kCardShadowColor = 0x12233B53;

constexpr float kLevelItemBottomSpacing = 10.0f;
constexpr float kLevelCardPadding = 6.0f;
constexpr float kLevelCardSpacing = 8.0f;
constexpr float kTopicItemExtent = 60.0f;
constexpr float kTopicRowHeight = 52.0f;
constexpr float kDisclosureAnimationDuration = 0.22f;
constexpr float kIconAnimationDuration = 0.18f;

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
        m_card->setBorderRadius(24.0f);
        m_card->setBackgroundColor(0xFFFFFFFF);
        m_card->setShadow(0.0f, 8.0f, 22.0f, 0.0f, 0x10233B53);
        addChild(m_card);

        m_headerRow = LFLinear::createHorizontal();
        m_headerRow->matchParentWidth();
        m_headerRow->wrapContentHeight();
        m_headerRow->setPadding(YGEdgeLeft, 18.0f);
        m_headerRow->setPadding(YGEdgeRight, 14.0f);
        m_headerRow->setPadding(YGEdgeTop, 14.0f);
        m_headerRow->setPadding(YGEdgeBottom, 14.0f);
        m_headerRow->setBorderRadius(18.0f);
        m_headerRow->setGravity(LFAlignment::Start, LFAlignment::Center);
        m_headerRow->setSpacing(14.0f);
        m_headerRow->setOnTap([this](const LFPoint&) {
            if (m_onToggle) {
                m_onToggle();
            }
        });
        m_card->addChild(m_headerRow);

        m_copy = LFLinear::createVertical();
        m_copy->setFlexGrow(1.0f);
        m_copy->setFlexBasis(0.0f);
        m_copy->wrapContentHeight();
        m_copy->setSpacing(8.0f);
        m_headerRow->addChild(m_copy);

        m_title = createText("", 17.0f, kTitleColor);
        m_title->matchParentWidth();
        m_title->setMaxLines(1);
        m_copy->addChild(m_title);

        m_metaPill = LFLinear::createHorizontal();
        m_metaPill->setWidth(70);
        m_metaPill->wrapContentHeight();
        m_metaPill->setPadding(YGEdgeLeft, 10.0f);
        m_metaPill->setPadding(YGEdgeRight, 10.0f);
        m_metaPill->setPadding(YGEdgeTop, 6.0f);
        m_metaPill->setPadding(YGEdgeBottom, 6.0f);
        m_metaPill->setBorderRadius(10.0f);
        m_metaPill->setGravity(LFAlignment::Center, LFAlignment::Center);
        m_copy->addChild(m_metaPill);

        m_meta = createText("", 11.5f, kMetaColor);
        m_meta->setMaxLines(1);
        m_meta->setTextHAlign(LFTextHAlign::Center);
        m_meta->setTextVAlign(LFTextVAlign::Center);
        m_metaPill->addChild(m_meta);

        m_iconBubble = LFBox::create();
        m_iconBubble->setWidth(42.0f);
        m_iconBubble->setHeight(42.0f);
        m_iconBubble->setBorderRadius(15.0f);
        m_iconBubble->setBorder(1.0f, 0x12D6E2EF);
        m_headerRow->addChild(m_iconBubble);

        m_icon = createImage("EnglishWordsAssets/Images/icon-add.png", 18.0f);
        m_iconBubble->addChild(m_icon, LFBoxAlign::Center);

        m_topicContainer = LFLinear::createVertical();
        m_topicContainer->matchParentWidth();
        m_topicContainer->wrapContentHeight();
        m_topicContainer->setPadding(YGEdgeTop, kLevelCardSpacing);
        m_topicContainer->setMasksToBounds(true);
        m_topicContainer->setHeight(0.0f);
        m_topicContainer->setOpacity(0.0f);
        m_topicContainer->setDisplay(YGDisplayNone);
        m_card->addChild(m_topicContainer);

        m_topicList = LFListView::createVertical();
        m_topicList->matchParentWidth();
        m_topicList->setScrollBarEnabled(false);
        m_topicList->setBounces(false);
        m_topicList->setMasksToBounds(true);
        m_topicContainer->addChild(m_topicList);
    }

    void bind(const EnglishWordLevel& level,
              const LevelStyle& style,
              std::function<void()> onToggle,
              TopicTapCallback onTopicTap) {
        m_style = style;
        m_onToggle = std::move(onToggle);

        m_title->setText(level.title);
        m_meta->setText(std::to_string(level.topics.size()) + " topics");
        m_topicListHeight = static_cast<float>(level.topics.size()) * kTopicItemExtent;
        m_topicList->setHeight(m_topicListHeight);
        m_topicList->setAdapter(std::make_shared<TopicListAdapter>(level, std::move(onTopicTap)));
        setExpanded(false, false);
    }

    void setBottomSpacing(float spacing) {
        setPadding(YGEdgeBottom, spacing);
    }

    void setExpanded(bool expanded, bool animated = true) {
        updateHeaderStyle(expanded);

        const float targetHeight = expanded ? (m_topicListHeight + kLevelCardSpacing) : 0.0f;
        const float targetRotation = expanded ? 45.0f : 0.0f;

        if (!animated) {
            m_topicContainerHeight = targetHeight;
            m_iconRotation = targetRotation;
            m_topicContainer->setDisplay(expanded ? YGDisplayFlex : YGDisplayNone);
            m_topicContainer->setHeight(targetHeight);
            m_topicContainer->setOpacity(expanded ? 1.0f : 0.0f);
            m_icon->setRotate(targetRotation);
            return;
        }

        if (m_topicContainerHeight == targetHeight && m_iconRotation == targetRotation) {
            return;
        }

        stopAnimations();
        m_topicContainer->setDisplay(YGDisplayFlex);
        const float alphaRange = std::max(m_topicContainerHeight, targetHeight);

        std::weak_ptr<LevelSectionView> weakSelf = std::static_pointer_cast<LevelSectionView>(shared_from_this());

        m_expandAnimator = LFValueAnimator<float>::of(m_topicContainerHeight, targetHeight);
        m_expandAnimator->setDuration(kDisclosureAnimationDuration);
        m_expandAnimator->setEasing(LFEasingType::QuadOut);
        m_expandAnimator->addUpdateListener([weakSelf, alphaRange](const float& height) {
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }

            self->m_topicContainerHeight = height;
            self->m_topicContainer->setHeight(height);
            const float alpha = alphaRange <= 0.0f
                ? 0.0f
                : std::clamp(height / alphaRange, 0.0f, 1.0f);
            self->m_topicContainer->setOpacity(alpha);
        });
        m_expandAnimator->setOnEnd([weakSelf, expanded, targetHeight]() {
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }

            self->m_topicContainerHeight = targetHeight;
            self->m_topicContainer->setHeight(targetHeight);
            self->m_topicContainer->setOpacity(expanded ? 1.0f : 0.0f);
            self->m_topicContainer->setDisplay(expanded ? YGDisplayFlex : YGDisplayNone);
            self->m_expandAnimator.reset();
        });

        m_iconAnimator = LFValueAnimator<float>::of(m_iconRotation, targetRotation);
        m_iconAnimator->setDuration(kIconAnimationDuration);
        m_iconAnimator->setEasing(LFEasingType::QuadOut);
        m_iconAnimator->addUpdateListener([weakSelf](const float& rotation) {
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }

            self->m_iconRotation = rotation;
            self->m_icon->setRotate(rotation);
        });
        m_iconAnimator->setOnEnd([weakSelf, targetRotation]() {
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }

            self->m_iconRotation = targetRotation;
            self->m_icon->setRotate(targetRotation);
            self->m_iconAnimator.reset();
        });

        m_expandAnimator->start();
        m_iconAnimator->start();
        LFGlobalAnimationManager::getInstance().addAnimator(m_expandAnimator);
        LFGlobalAnimationManager::getInstance().addAnimator(m_iconAnimator);
    }

private:
    void updateHeaderStyle(bool expanded) {
        if (expanded) {
            m_card->setBorder(1.5f, m_style.accentColor);
            m_card->setShadow(0.0f, 10.0f, 26.0f, 0.0f, 0x163567A3);
            m_headerRow->setBackgroundColor(m_style.tintColor);
            m_metaPill->setBackgroundColor(0xFFFFFFFF);
            m_iconBubble->setBackgroundColor(0xFFFFFFFF);
            m_iconBubble->setBorder(1.0f, m_style.accentColor);
        } else {
            m_card->setBorder(1.0f, kCardBorderColor);
            m_card->setShadow(0.0f, 8.0f, 22.0f, 0.0f, 0x10233B53);
            m_headerRow->setBackgroundColor(m_style.tintColor);
            m_metaPill->setBackgroundColor(0xFFFFFFFF);
            m_iconBubble->setBackgroundColor(0xFFFFFFFF);
            m_iconBubble->setBorder(1.0f, 0x12D6E2EF);
        }
    }

    void stopAnimations() {
        if (m_expandAnimator) {
            m_expandAnimator->cancel();
            LFGlobalAnimationManager::getInstance().removeAnimator(m_expandAnimator);
            m_expandAnimator.reset();
        }
        if (m_iconAnimator) {
            m_iconAnimator->cancel();
            LFGlobalAnimationManager::getInstance().removeAnimator(m_iconAnimator);
            m_iconAnimator.reset();
        }
    }

    LevelStyle m_style;
    float m_topicListHeight = 0.0f;
    float m_topicContainerHeight = 0.0f;
    float m_iconRotation = 0.0f;
    std::function<void()> m_onToggle;
    std::shared_ptr<LFLinear> m_card;
    std::shared_ptr<LFLinear> m_headerRow;
    std::shared_ptr<LFLinear> m_copy;
    std::shared_ptr<LFText> m_title;
    std::shared_ptr<LFLinear> m_metaPill;
    std::shared_ptr<LFText> m_meta;
    std::shared_ptr<LFBox> m_iconBubble;
    std::shared_ptr<LFImage> m_icon;
    std::shared_ptr<LFLinear> m_topicContainer;
    std::shared_ptr<LFListView> m_topicList;
    std::shared_ptr<LFValueAnimator<float>> m_expandAnimator;
    std::shared_ptr<LFValueAnimator<float>> m_iconAnimator;
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
    m_content->setMargin(YGEdgeBottom, 20.0f);
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
        section->setBottomSpacing(index == static_cast<int>(m_levels.size()) - 1 ? 0.0f : kLevelItemBottomSpacing);
        sections->push_back(section);
        m_content->addChild(section);
    }

    for (int index = 0; index < static_cast<int>(sections->size()); ++index) {
        (*sections)[static_cast<size_t>(index)]->setExpanded(index == *expandedIndex, false);
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
