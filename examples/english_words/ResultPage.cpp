#include "ResultPage.h"

#include "EnglishWordsI18n.h"
#include "PointPage.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace {

constexpr uint32_t kPageBackgroundColor = 0xFFF4F7FB;
constexpr uint32_t kTitleColor = 0xFF142033;
constexpr uint32_t kSubtitleColor = 0xFF6B7A90;
constexpr uint32_t kMetaColor = 0xFF4E6178;
constexpr uint32_t kSurfaceColor = 0xFFEAF1F8;
constexpr uint32_t kSurfaceBorderColor = 0xFFDDE7F1;
constexpr uint32_t kCardBorderColor = 0xFFDCE6F2;
constexpr uint32_t kCardShadowColor = 0x12233B53;

constexpr float kItemExtent = 144.0f;
constexpr float kItemContentHeight = 132.0f;

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

std::string formatScore(double score) {
    const double rounded = std::round(score * 10.0) / 10.0;
    if (std::fabs(rounded - std::round(rounded)) < 0.001) {
        return std::to_string(static_cast<int>(std::round(rounded)));
    }

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(1) << rounded;
    return stream.str();
}

class ResultListItemView : public LFLinear {
public:
    using Ptr = std::shared_ptr<ResultListItemView>;

    static Ptr create() {
        return std::make_shared<ResultListItemView>();
    }

    ResultListItemView() {
        matchParentWidth();
        wrapContentHeight();
        setPadding(YGEdgeLeft, 8.0f);
        setPadding(YGEdgeRight, 8.0f);
        setPadding(YGEdgeTop, 8.0f);

        m_baseLayer = LFBox::create();
        m_baseLayer->matchParentWidth();
        m_baseLayer->setHeight(kItemContentHeight);
        addChild(m_baseLayer);

        m_verticalLinear = LFLinear::createVertical();
        m_verticalLinear->matchParentWidth();
        m_verticalLinear->matchParentHeight();
        m_verticalLinear->setPadding(YGEdgeAll, 16.0f);
        m_verticalLinear->setSpacing(12.0f);
        m_verticalLinear->setBorderRadius(20.0f);
        m_verticalLinear->setBorder(1.0f, kCardBorderColor);
        m_verticalLinear->setBackgroundColor(0xFFFFFFFF);
        m_verticalLinear->setShadow(0.0f, 8.0f, 20.0f, 0.0f, kCardShadowColor);
        m_verticalLinear->setOnTap([this](const LFPoint&) {
            if (m_onTap) {
                m_onTap(m_summary);
            }
        });
        m_verticalLinear->setGravity(LFAlignment::End, LFAlignment::Start);
        m_baseLayer->addChild(m_verticalLinear, LFBoxAlign::Center);

        m_scorePill = LFLinear::createHorizontal();
        m_scorePill->wrapContentWidth();
        m_scorePill->wrapContentHeight();
        m_scorePill->setPadding(YGEdgeLeft, 10.0f);
        m_scorePill->setPadding(YGEdgeRight, 10.0f);
        m_scorePill->setPadding(YGEdgeTop, 8.0f);
        m_scorePill->setPadding(YGEdgeBottom, 8.0f);
        m_scorePill->setBorderRadius(10.0f);
        m_scorePill->setBackgroundColor(0xFFEAF3FF);
        m_scorePill->setGravity(LFAlignment::Center, LFAlignment::Center);
        m_baseLayer->addChild(m_scorePill, LFBoxAlign::TopRight, 16, 16);

        m_timeText = createText("", 12.0f, kSubtitleColor);
        m_timeText->setMaxLines(1);
        m_timeText->setTextHAlign(LFTextHAlign::Left);
        m_timeText->setTextVAlign(LFTextVAlign::Center);
        m_baseLayer->addChild(m_timeText, LFBoxAlign::TopLeft, 16, 16);

        m_scoreText = createText("", 12.0f, 0xFF3567A3);
        m_scoreText->setMaxLines(1);
        m_scoreText->setTextHAlign(LFTextHAlign::Center);
        m_scoreText->setTextVAlign(LFTextVAlign::Center);
        m_scorePill->addChild(m_scoreText);

        m_levelText = createText("", 14.0f, kMetaColor);
        m_levelText->matchParentWidth();
        m_levelText->setMaxLines(1);
        m_verticalLinear->addChild(m_levelText);

        m_topicText = createText("", 16.0f, kTitleColor);
        m_topicText->matchParentWidth();
        m_topicText->setMaxLines(1);
        m_verticalLinear->addChild(m_topicText);

        m_modeText = createText("", 13.0f, kSubtitleColor);
        m_modeText->matchParentWidth();
        m_modeText->setMaxLines(1);
        m_verticalLinear->addChild(m_modeText);
    }

    void bindResult(const EnglishWordsSavedResultSummary& summary,
                    std::function<void(const EnglishWordsSavedResultSummary&)> onTap) {
        m_summary = summary;
        m_onTap = std::move(onTap);

        m_verticalLinear->setBackgroundColor(0xFFFFFFFF);
        m_timeText->setText(summary.createdAt);
        m_scoreText->setText(EnglishWordsI18n::scoreRow(
            formatScore(summary.totalScore),
            summary.questionCount
        ));
        m_levelText->setText(summary.levelTitle.empty() ? fallbackLevelText(summary.levelId) : summary.levelTitle);
        m_topicText->setText(summary.topic.title);
        m_topicText->setTextHAlign(LFTextHAlign::Left);
        m_modeText->setText(EnglishWordsI18n::modeLabel(summary.mode));

        m_timeText->setDisplay(YGDisplayFlex);
        m_scorePill->setDisplay(YGDisplayFlex);
        m_levelText->setDisplay(YGDisplayFlex);
        m_topicText->setDisplay(YGDisplayFlex);
        m_modeText->setDisplay(YGDisplayFlex);
    }

    void bindMessage(const std::string& text) {
        m_summary = EnglishWordsSavedResultSummary{};
        m_onTap = nullptr;

        m_verticalLinear->setBackgroundColor(0xFFFFFFFF);
        m_topicText->setText(text);
        m_topicText->setTextHAlign(LFTextHAlign::Center);

        m_timeText->setDisplay(YGDisplayNone);
        m_scorePill->setDisplay(YGDisplayNone);
        m_levelText->setDisplay(YGDisplayNone);
        m_modeText->setDisplay(YGDisplayNone);
    }

private:
    static std::string fallbackLevelText(const std::string& levelId) {
        return levelId.empty()
            ? EnglishWordsI18n::tr("english_words.result.unknown_level")
            : EnglishWordsI18n::levelTitle(levelId);
    }

    EnglishWordsSavedResultSummary m_summary;
    std::function<void(const EnglishWordsSavedResultSummary&)> m_onTap;
    std::shared_ptr<LFBox> m_baseLayer;
    std::shared_ptr<LFLinear> m_verticalLinear;
    std::shared_ptr<LFText> m_timeText;
    std::shared_ptr<LFLinear> m_scorePill;
    std::shared_ptr<LFText> m_scoreText;
    std::shared_ptr<LFText> m_levelText;
    std::shared_ptr<LFText> m_topicText;
    std::shared_ptr<LFText> m_modeText;
};

class SavedResultsAdapter : public LFListAdapter {
public:
    SavedResultsAdapter(std::function<const std::vector<EnglishWordsSavedResultSummary>*()> resultsProvider,
                        std::function<std::string()> statusProvider,
                        std::function<void(const EnglishWordsSavedResultSummary&)> onTap)
        : m_resultsProvider(std::move(resultsProvider)),
          m_statusProvider(std::move(statusProvider)),
          m_onTap(std::move(onTap)) {
    }

    int getCount() override {
        const auto* results = m_resultsProvider ? m_resultsProvider() : nullptr;
        return (!results || results->empty()) ? 1 : static_cast<int>(results->size());
    }

    LFNode::Ptr createView() override {
        return ResultListItemView::create();
    }

    void bindView(LFNode::Ptr view, int index) override {
        auto item = std::static_pointer_cast<ResultListItemView>(view);
        if (!item) {
            return;
        }

        const auto* results = m_resultsProvider ? m_resultsProvider() : nullptr;
        if (!results || results->empty() || index < 0 || index >= static_cast<int>(results->size())) {
            item->setMargin(YGEdgeBottom, 0.0f);
            item->bindMessage(m_statusProvider ? m_statusProvider() : EnglishWordsI18n::tr("english_words.result.empty"));
            return;
        }

        item->setMargin(YGEdgeBottom, index == static_cast<int>(results->size()) - 1 ? 8.0f : 0.0f);
        item->bindResult((*results)[static_cast<size_t>(index)], m_onTap);
    }

    float getItemExtent(int index) override {
        (void)index;
        return kItemExtent;
    }

private:
    std::function<const std::vector<EnglishWordsSavedResultSummary>*()> m_resultsProvider;
    std::function<std::string()> m_statusProvider;
    std::function<void(const EnglishWordsSavedResultSummary&)> m_onTap;
};

} // namespace

std::shared_ptr<ResultPage> ResultPage::create() {
    auto page = std::make_shared<ResultPage>();
    page->m_dataManager = EnglishWordsDataManager::create();
    page->setBackgroundColor(kPageBackgroundColor);
    page->buildUI();
    page->loadResults();
    return page;
}

void ResultPage::buildUI() {
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

    auto leftSlot = LFLinear::createHorizontal();
    leftSlot->setWidth(78.0f);
    leftSlot->setHeight(46.0f);
    leftSlot->setGravity(LFAlignment::Start, LFAlignment::Center);
    headerRow->addChild(leftSlot);

    auto backButton = LFLinear::createHorizontal();
    backButton->setWidth(46.0f);
    backButton->setHeight(46.0f);
    backButton->setBorderRadius(16.0f);
    backButton->setBorder(1.0f, 0xFFD8E4F1);
    backButton->setBackgroundColor(0xFFFFFFFF);
    backButton->setShadow(0.0f, 6.0f, 18.0f, 0.0f, 0x10233B53);
    backButton->setGravity(LFAlignment::Center, LFAlignment::Center);
    backButton->addChild(createImage("EnglishWordsAssets/Images/icon-arrow-left.png", 18.0f));
    leftSlot->addChild(backButton);

    std::weak_ptr<ResultPage> weakSelf = std::static_pointer_cast<ResultPage>(shared_from_this());
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

    auto title = createText(EnglishWordsI18n::tr("english_words.result.title"), 22.0f, kTitleColor);
    title->matchParentWidth();
    title->setTextHAlign(LFTextHAlign::Center);
    title->setMaxLines(1);
    titleWrap->addChild(title);

    auto selectButton = LFButton::create(EnglishWordsI18n::tr("english_words.result.options"));
    selectButton->setWidth(78.0f);
    selectButton->setHeight(46.0f);
    selectButton->setBorderRadius(16.0f);
    selectButton->setBorder(1.0f, 0xFFD8E4F1);
    selectButton->setFontSize(14.0f);
    selectButton->setTextColor(0xFF21354D);
    selectButton->setBackgroundColor(LFButtonState::Normal, 0xFFFFFFFF);
    selectButton->setBackgroundColor(LFButtonState::Pressed, 0xFFF3F7FB);
    selectButton->setShadow(0.0f, 6.0f, 18.0f, 0.0f, 0x10233B53);
    headerRow->addChild(selectButton);

    m_listView = LFListView::createVertical();
    m_listView->matchParentWidth();
    m_listView->setFlexGrow(1.0f);
    m_listView->setFlexBasis(0.0f);
    m_listView->setScrollBarEnabled(false);
    m_listView->setBounces(false);
    m_listView->setBackgroundColor(kSurfaceColor);
    m_listView->setBorderRadius(28.0f);
    m_listView->setBorder(1.0f, kSurfaceBorderColor);
    m_listView->setMargin(YGEdgeBottom, 20.0f);
    root->addChild(m_listView);

    m_listView->setAdapter(std::make_shared<SavedResultsAdapter>(
        [this]() -> const std::vector<EnglishWordsSavedResultSummary>* { return &m_results; },
        [this]() { return m_statusMessage; },
        [this](const EnglishWordsSavedResultSummary& summary) {
            openResult(summary);
        }
    ));
}

void ResultPage::loadResults() {
    showStatus(EnglishWordsI18n::tr("english_words.result.loading"));

    if (!m_dataManager) {
        showStatus(EnglishWordsI18n::tr("english_words.result.failed"));
        return;
    }

    std::weak_ptr<ResultPage> weakSelf = std::static_pointer_cast<ResultPage>(shared_from_this());
    m_dataManager->loadSavedResults([weakSelf](bool ok,
                                               std::vector<EnglishWordsSavedResultSummary> results,
                                               const std::string&) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }

        if (!ok) {
            self->showStatus(EnglishWordsI18n::tr("english_words.result.failed"));
            return;
        }

        self->m_results = std::move(results);
        self->m_statusMessage = self->m_results.empty()
            ? EnglishWordsI18n::tr("english_words.result.empty")
            : "";
        self->refreshList();
    });
}

void ResultPage::refreshList() {
    if (m_listView) {
        m_listView->notifyDataSetChanged();
    }
}

void ResultPage::showStatus(const std::string& text) {
    m_results.clear();
    m_statusMessage = text;
    refreshList();
}

void ResultPage::openResult(const EnglishWordsSavedResultSummary& summary) {
    if (!m_dataManager || summary.fileName.empty()) {
        return;
    }

    std::weak_ptr<ResultPage> weakSelf = std::static_pointer_cast<ResultPage>(shared_from_this());
    m_dataManager->loadExamResult(summary.fileName, [weakSelf](bool ok, EnglishWordsExamResult result, const std::string&) {
        auto self = weakSelf.lock();
        if (!self || !ok) {
            return;
        }

        if (auto navigator = self->getNavigator()) {
            navigator->push(PointPage::create(result));
        }
    });
}
