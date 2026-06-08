#include "ResultPage.h"

#include "EnglishWordsI18n.h"
#include "LFCheckbox.h"
#include "PointPage.h"

#include <algorithm>
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
constexpr uint32_t kSelectionAccentColor = 0xFF3567A3;
constexpr uint32_t kSelectionCardColor = 0xFFF7FBFF;
constexpr uint32_t kSelectionBorderColor = 0xFFBBD1EA;

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

std::shared_ptr<LFButton> createHeaderButton(const std::string& text, float width) {
    auto button = LFButton::create(text);
    button->setWidth(width);
    button->setHeight(46.0f);
    button->setBorderRadius(16.0f);
    button->setBorder(1.0f, 0xFFD8E4F1);
    button->setFontSize(14.0f);
    button->setTextColor(0xFF21354D);
    button->setBackgroundColor(LFButtonState::Normal, 0xFFFFFFFF);
    button->setBackgroundColor(LFButtonState::Pressed, 0xFFF3F7FB);
    button->setBackgroundColor(LFButtonState::Disabled, 0xFFF3F6FA);
    button->setShadow(0.0f, 6.0f, 18.0f, 0.0f, 0x10233B53);
    return button;
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
        m_verticalLinear->setGravity(LFAlignment::End, LFAlignment::Start);
        m_verticalLinear->setOnTap([this](const LFPoint&) {
            if (m_onTap) {
                m_onTap(m_summary);
            }
        });
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

        m_checkbox = LFCheckbox::create("", false);
        m_checkbox->setTouchEnabled(false);
        m_checkbox->setHitTestEnabled(false);
        m_checkbox->setBoxSize(20.0f);
        m_checkbox->setCornerRadius(6.0f);
        m_checkbox->setBorderWidth(1.5f);
        m_checkbox->setIndicatorColor(0xFFFFFFFF, kSelectionAccentColor);
        m_checkbox->setBorderColor(0xFFC8D6E5, kSelectionAccentColor);
        m_checkbox->setCheckmarkColor(0xFFFFFFFF);
        m_checkbox->setDisplay(YGDisplayNone);
        m_baseLayer->addChild(m_checkbox, LFBoxAlign::BottomRight, 8.0f, 16.0f);

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
                    bool selectionMode,
                    bool selected,
                    std::function<void(const EnglishWordsSavedResultSummary&)> onTap) {
        m_summary = summary;
        m_onTap = std::move(onTap);

        m_verticalLinear->setBackgroundColor(selectionMode && selected ? kSelectionCardColor : 0xFFFFFFFF);
        m_verticalLinear->setBorder(1.0f, selectionMode && selected ? kSelectionBorderColor : kCardBorderColor);
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
        m_checkbox->setDisplay(selectionMode ? YGDisplayFlex : YGDisplayNone);
        m_checkbox->setChecked(selected, false);
    }

    void bindMessage(const std::string& text) {
        m_summary = EnglishWordsSavedResultSummary{};
        m_onTap = nullptr;

        m_verticalLinear->setBackgroundColor(0xFFFFFFFF);
        m_verticalLinear->setBorder(1.0f, kCardBorderColor);
        m_topicText->setText(text);
        m_topicText->setTextHAlign(LFTextHAlign::Center);

        m_timeText->setDisplay(YGDisplayNone);
        m_scorePill->setDisplay(YGDisplayNone);
        m_levelText->setDisplay(YGDisplayNone);
        m_modeText->setDisplay(YGDisplayNone);
        m_checkbox->setDisplay(YGDisplayNone);
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
    std::shared_ptr<LFCheckbox> m_checkbox;
};

class SavedResultsAdapter : public LFListAdapter {
public:
    SavedResultsAdapter(std::function<const std::vector<EnglishWordsSavedResultSummary>*()> resultsProvider,
                        std::function<std::string()> statusProvider,
                        std::function<bool()> selectionModeProvider,
                        std::function<bool(const std::string&)> selectionProvider,
                        std::function<void(const EnglishWordsSavedResultSummary&)> onTap)
        : m_resultsProvider(std::move(resultsProvider)),
          m_statusProvider(std::move(statusProvider)),
          m_selectionModeProvider(std::move(selectionModeProvider)),
          m_selectionProvider(std::move(selectionProvider)),
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

        const auto& summary = (*results)[static_cast<size_t>(index)];
        item->setMargin(YGEdgeBottom, index == static_cast<int>(results->size()) - 1 ? 8.0f : 0.0f);
        item->bindResult(
            summary,
            m_selectionModeProvider ? m_selectionModeProvider() : false,
            m_selectionProvider ? m_selectionProvider(summary.resultId) : false,
            m_onTap
        );
    }

    float getItemExtent(int index) override {
        (void)index;
        return kItemExtent;
    }

private:
    std::function<const std::vector<EnglishWordsSavedResultSummary>*()> m_resultsProvider;
    std::function<std::string()> m_statusProvider;
    std::function<bool()> m_selectionModeProvider;
    std::function<bool(const std::string&)> m_selectionProvider;
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

    m_leftSlot = LFLinear::createHorizontal();
    m_leftSlot->setWidth(112.0f);
    m_leftSlot->setHeight(46.0f);
    m_leftSlot->setGravity(LFAlignment::Start, LFAlignment::Center);
    headerRow->addChild(m_leftSlot);

    m_backButton = LFLinear::createHorizontal();
    m_backButton->setWidth(46.0f);
    m_backButton->setHeight(46.0f);
    m_backButton->setBorderRadius(16.0f);
    m_backButton->setBorder(1.0f, 0xFFD8E4F1);
    m_backButton->setBackgroundColor(0xFFFFFFFF);
    m_backButton->setShadow(0.0f, 6.0f, 18.0f, 0.0f, 0x10233B53);
    m_backButton->setGravity(LFAlignment::Center, LFAlignment::Center);
    m_backButton->addChild(createImage("EnglishWordsAssets/Images/icon-arrow-left.png", 18.0f));
    m_leftSlot->addChild(m_backButton);

    std::weak_ptr<ResultPage> weakSelf = std::static_pointer_cast<ResultPage>(shared_from_this());
    m_backButton->setOnTap([weakSelf](const LFPoint&) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        if (auto navigator = self->getNavigator()) {
            navigator->pop();
        }
    });

    m_cancelButton = createHeaderButton(EnglishWordsI18n::tr("english_words.result.cancel"), 78.0f);
    m_cancelButton->setDisplay(YGDisplayNone);
    m_cancelButton->setOnClick([weakSelf](LFButton*) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        self->exitSelectionMode();
    });
    m_leftSlot->addChild(m_cancelButton);

    auto titleWrap = LFLinear::createVertical();
    titleWrap->setFlexGrow(1.0f);
    titleWrap->setFlexBasis(0.0f);
    titleWrap->wrapContentHeight();
    headerRow->addChild(titleWrap);

    m_titleText = createText(EnglishWordsI18n::tr("english_words.result.title"), 22.0f, kTitleColor);
    m_titleText->matchParentWidth();
    m_titleText->setTextHAlign(LFTextHAlign::Center);
    m_titleText->setMaxLines(1);
    titleWrap->addChild(m_titleText);

    auto rightSlot = LFLinear::createHorizontal();
    rightSlot->setWidth(112.0f);
    rightSlot->setHeight(46.0f);
    rightSlot->setGravity(LFAlignment::End, LFAlignment::Center);
    headerRow->addChild(rightSlot);

    m_selectButton = createHeaderButton(EnglishWordsI18n::tr("english_words.result.options"), 78.0f);
    m_selectButton->setOnClick([weakSelf](LFButton*) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        self->enterSelectionMode();
    });
    rightSlot->addChild(m_selectButton);

    m_selectAllButton = createHeaderButton(EnglishWordsI18n::tr("english_words.result.select_all"), 112.0f);
    m_selectAllButton->setDisplay(YGDisplayNone);
    m_selectAllButton->setOnClick([weakSelf](LFButton*) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }

        const bool allSelected = !self->m_results.empty() &&
            self->m_selectedResultIds.size() == self->m_results.size();
        self->setAllResultsSelected(!allSelected);
    });
    rightSlot->addChild(m_selectAllButton);

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
        [this]() { return m_selectionMode; },
        [this](const std::string& resultId) { return isResultSelected(resultId); },
        [this](const EnglishWordsSavedResultSummary& summary) {
            if (m_selectionMode) {
                toggleResultSelection(summary);
                return;
            }
            openResult(summary);
        }
    ));

    m_deleteButton = LFButton::create(EnglishWordsI18n::tr("english_words.result.delete"));
    m_deleteButton->setWidth(112.0f);
    m_deleteButton->setHeight(52.0f);
    m_deleteButton->setBorderRadius(20.0f);
    m_deleteButton->setBorder(1.0f, kSelectionAccentColor);
    m_deleteButton->setFontSize(15.0f);
    m_deleteButton->setTextColor(0xFFFFFFFF);
    m_deleteButton->setBackgroundColor(LFButtonState::Normal, kSelectionAccentColor);
    m_deleteButton->setBackgroundColor(LFButtonState::Pressed, 0xFF2C598F);
    m_deleteButton->setBackgroundColor(LFButtonState::Disabled, 0xFFAEBFD3);
    m_deleteButton->setShadow(0.0f, 10.0f, 24.0f, 0.0f, 0x16233B53);
    m_deleteButton->setPositionType(YGPositionTypeAbsolute);
    m_deleteButton->setPosition(YGEdgeRight, 20.0f);
    m_deleteButton->setPosition(YGEdgeBottom, 20.0f);
    m_deleteButton->setDisplay(YGDisplayNone);
    m_deleteButton->setOnClick([weakSelf](LFButton*) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        self->confirmDeleteSelected();
    });
    root->addChild(m_deleteButton);

    updateSelectionHeader();
    updateDeleteButton();
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

        std::set<std::string> nextSelectedIds;
        for (const auto& summary : self->m_results) {
            if (self->m_selectedResultIds.find(summary.resultId) != self->m_selectedResultIds.end()) {
                nextSelectedIds.insert(summary.resultId);
            }
        }
        self->m_selectedResultIds = std::move(nextSelectedIds);

        if (self->m_results.empty()) {
            self->m_selectionMode = false;
        }

        self->m_statusMessage = self->m_results.empty()
            ? EnglishWordsI18n::tr("english_words.result.empty")
            : "";
        self->refreshList();
    });
}

void ResultPage::refreshList() {
    updateSelectionHeader();
    updateDeleteButton();
    if (m_listView) {
        m_listView->notifyDataSetChanged();
    }
}

void ResultPage::showStatus(const std::string& text) {
    m_results.clear();
    m_selectedResultIds.clear();
    m_selectionMode = false;
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

void ResultPage::enterSelectionMode() {
    if (m_results.empty()) {
        return;
    }

    m_selectionMode = true;
    m_selectedResultIds.clear();
    refreshList();
}

void ResultPage::exitSelectionMode() {
    m_selectionMode = false;
    m_selectedResultIds.clear();
    refreshList();
}

void ResultPage::updateSelectionHeader() {
    if (m_titleText) {
        m_titleText->setText(
            m_selectionMode
                ? EnglishWordsI18n::format("english_words.result.selected_count", {
                    {"count", std::to_string(m_selectedResultIds.size())}
                })
                : EnglishWordsI18n::tr("english_words.result.title")
        );
    }

    if (m_backButton) {
        m_backButton->setDisplay(m_selectionMode ? YGDisplayNone : YGDisplayFlex);
    }

    if (m_cancelButton) {
        m_cancelButton->setText(EnglishWordsI18n::tr("english_words.result.cancel"));
        m_cancelButton->setDisplay(m_selectionMode ? YGDisplayFlex : YGDisplayNone);
    }

    if (m_selectButton) {
        m_selectButton->setText(EnglishWordsI18n::tr("english_words.result.options"));
        m_selectButton->setDisplay(m_selectionMode ? YGDisplayNone : YGDisplayFlex);
        m_selectButton->setEnabled(!m_results.empty());
    }

    if (m_selectAllButton) {
        const bool hasResults = !m_results.empty();
        const bool allSelected = hasResults && m_selectedResultIds.size() == m_results.size();
        m_selectAllButton->setText(EnglishWordsI18n::tr(
            allSelected
                ? "english_words.result.clear_all"
                : "english_words.result.select_all"
        ));
        m_selectAllButton->setDisplay(m_selectionMode ? YGDisplayFlex : YGDisplayNone);
        m_selectAllButton->setEnabled(hasResults);
    }
}

void ResultPage::updateDeleteButton() {
    if (m_listView) {
        m_listView->setMargin(YGEdgeBottom, m_selectionMode ? 96.0f : 20.0f);
    }

    if (!m_deleteButton) {
        return;
    }

    const bool showDeleteButton = m_selectionMode;
    m_deleteButton->setText(EnglishWordsI18n::tr("english_words.result.delete"));
    m_deleteButton->setDisplay(showDeleteButton ? YGDisplayFlex : YGDisplayNone);
    m_deleteButton->setVisible(showDeleteButton);
    m_deleteButton->setEnabled(showDeleteButton && !m_selectedResultIds.empty());
}

void ResultPage::toggleResultSelection(const EnglishWordsSavedResultSummary& summary) {
    if (summary.resultId.empty()) {
        return;
    }

    const auto found = m_selectedResultIds.find(summary.resultId);
    if (found == m_selectedResultIds.end()) {
        m_selectedResultIds.insert(summary.resultId);
    } else {
        m_selectedResultIds.erase(found);
    }
    refreshList();
}

void ResultPage::setAllResultsSelected(bool selected) {
    m_selectedResultIds.clear();
    if (selected) {
        for (const auto& summary : m_results) {
            if (!summary.resultId.empty()) {
                m_selectedResultIds.insert(summary.resultId);
            }
        }
    }
    refreshList();
}

bool ResultPage::isResultSelected(const std::string& resultId) const {
    return !resultId.empty() && m_selectedResultIds.find(resultId) != m_selectedResultIds.end();
}

void ResultPage::confirmDeleteSelected() {
    deleteSelectedResults();
}

void ResultPage::deleteSelectedResults() {
    if (!m_dataManager || m_selectedResultIds.empty()) {
        return;
    }

    std::vector<std::string> resultIds(m_selectedResultIds.begin(), m_selectedResultIds.end());
    if (m_deleteButton) {
        m_deleteButton->setEnabled(false);
    }

    std::weak_ptr<ResultPage> weakSelf = std::static_pointer_cast<ResultPage>(shared_from_this());
    m_dataManager->deleteSavedResults(resultIds, [weakSelf, resultIds](bool ok, const std::string&) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }

        if (!ok) {
            self->updateDeleteButton();
            return;
        }

        std::set<std::string> deletedIds(resultIds.begin(), resultIds.end());
        std::vector<EnglishWordsSavedResultSummary> remainingResults;
        remainingResults.reserve(self->m_results.size());
        for (const auto& summary : self->m_results) {
            if (deletedIds.find(summary.resultId) == deletedIds.end()) {
                remainingResults.push_back(summary);
            }
        }

        self->m_results = std::move(remainingResults);
        self->m_statusMessage = self->m_results.empty()
            ? EnglishWordsI18n::tr("english_words.result.empty")
            : "";
        self->exitSelectionMode();
    });
}
