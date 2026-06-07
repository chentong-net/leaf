#include "PointPage.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

namespace {

constexpr uint32_t kPageBackgroundColor = 0xFFF4F7FB;
constexpr uint32_t kTitleColor = 0xFF142033;
constexpr uint32_t kSubtitleColor = 0xFF6B7A90;
constexpr uint32_t kCardBorderColor = 0xFFDCE6F2;
constexpr uint32_t kCardShadowColor = 0x12233B53;

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

std::string modeLabel(EnglishWordsTestMode mode) {
    switch (mode) {
        case EnglishWordsTestMode::ChineseToEnglish: return "Chinese -> English";
        case EnglishWordsTestMode::RussianToEnglish: return "Russian -> English";
        case EnglishWordsTestMode::EnglishToChinese: return "English -> Chinese";
        case EnglishWordsTestMode::EnglishToRussian: return "English -> Russian";
        case EnglishWordsTestMode::AudioToChinese: return "Audio -> Chinese";
        case EnglishWordsTestMode::AudioToRussian: return "Audio -> Russian";
        case EnglishWordsTestMode::AudioToEnglish: return "Audio -> English";
    }

    return "Exam";
}

std::string questionPrompt(const EnglishWordsQuestionResult& result) {
    switch (result.mode) {
        case EnglishWordsTestMode::ChineseToEnglish:
            return !result.entry.chineseTranslation.empty() ? result.entry.chineseTranslation : result.entry.text;
        case EnglishWordsTestMode::RussianToEnglish:
            return !result.entry.russianTranslation.empty() ? result.entry.russianTranslation : result.entry.text;
        case EnglishWordsTestMode::EnglishToChinese:
        case EnglishWordsTestMode::EnglishToRussian:
            return result.entry.text;
        case EnglishWordsTestMode::AudioToChinese:
        case EnglishWordsTestMode::AudioToRussian:
        case EnglishWordsTestMode::AudioToEnglish:
            return "Audio: " + result.entry.text;
    }

    return result.entry.text;
}

std::string questionStatus(double score) {
    if (score >= 0.99) {
        return "True";
    }
    if (score > 0.0) {
        return "Half";
    }
    return "False";
}

} // namespace

std::shared_ptr<PointPage> PointPage::create(const EnglishWordsExamResult& result) {
    auto page = std::make_shared<PointPage>();
    page->m_result = result;
    page->setBackgroundColor(kPageBackgroundColor);
    page->buildUI();
    return page;
}

void PointPage::buildUI() {
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

    std::weak_ptr<PointPage> weakSelf = std::static_pointer_cast<PointPage>(shared_from_this());
    backButton->setOnTap([weakSelf](const LFPoint&) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        self->goHome();
    });

    auto titleWrap = LFLinear::createVertical();
    titleWrap->setFlexGrow(1.0f);
    titleWrap->setFlexBasis(0.0f);
    titleWrap->wrapContentHeight();
    headerRow->addChild(titleWrap);

    auto title = createText("Result", 22.0f, kTitleColor);
    title->matchParentWidth();
    title->setTextHAlign(LFTextHAlign::Center);
    title->setMaxLines(1);
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

    auto content = LFLinear::createVertical();
    content->matchParentWidth();
    content->wrapContentHeight();
    content->setPadding(YGEdgeAll, 24.0f);
    content->setSpacing(16.0f);
    content->setMargin(YGEdgeBottom, 20.0f);
    scrollView->addChild(content);

    auto summaryCard = LFLinear::createVertical();
    summaryCard->matchParentWidth();
    summaryCard->wrapContentHeight();
    summaryCard->setPadding(YGEdgeAll, 24.0f);
    summaryCard->setSpacing(10.0f);
    summaryCard->setBackgroundColor(0xFFFFFFFF);
    summaryCard->setBorderRadius(30.0f);
    summaryCard->setBorder(1.0f, kCardBorderColor);
    summaryCard->setShadow(0.0f, 12.0f, 28.0f, 0.0f, kCardShadowColor);
    content->addChild(summaryCard);

    auto scoreTitle = createText("Score", 16.0f, kSubtitleColor);
    scoreTitle->matchParentWidth();
    scoreTitle->setTextHAlign(LFTextHAlign::Center);
    summaryCard->addChild(scoreTitle);

    auto scoreValue = createText(
        formatScore(m_result.totalScore) + "/" + std::to_string(m_result.questionCount),
        34.0f,
        kTitleColor
    );
    scoreValue->matchParentWidth();
    scoreValue->setTextHAlign(LFTextHAlign::Center);
    summaryCard->addChild(scoreValue);

    auto summaryMeta = createText(
        m_result.topic.title + "  " + modeLabel(m_result.mode),
        13.0f,
        kSubtitleColor
    );
    summaryMeta->matchParentWidth();
    summaryMeta->setTextHAlign(LFTextHAlign::Center);
    summaryMeta->setMaxLines(2);
    summaryCard->addChild(summaryMeta);

    auto listCard = LFLinear::createVertical();
    listCard->matchParentWidth();
    listCard->wrapContentHeight();
    listCard->setPadding(YGEdgeAll, 10.0f);
    listCard->setSpacing(10.0f);
    listCard->setBackgroundColor(0xFFEAF1F8);
    listCard->setBorderRadius(28.0f);
    listCard->setBorder(1.0f, 0xFFDDE7F1);
    content->addChild(listCard);

    for (size_t index = 0; index < m_result.questions.size(); ++index) {
        const auto& question = m_result.questions[index];

        auto rowCard = LFLinear::createVertical();
        rowCard->matchParentWidth();
        rowCard->wrapContentHeight();
        rowCard->setPadding(YGEdgeAll, 16.0f);
        rowCard->setSpacing(6.0f);
        rowCard->setBackgroundColor(0xFFFFFFFF);
        rowCard->setBorderRadius(20.0f);
        rowCard->setBorder(1.0f, kCardBorderColor);
        listCard->addChild(rowCard);

        auto line = createText(
            std::to_string(index + 1) + ". " + questionPrompt(question) + " - " + questionStatus(question.score),
            14.0f,
            kTitleColor
        );
        line->matchParentWidth();
        rowCard->addChild(line);

        if (question.score < 0.99) {
            auto answer = createText(
                "Your answer: " + (question.userAnswer.empty() ? "(empty)" : question.userAnswer),
                12.0f,
                kSubtitleColor
            );
            answer->matchParentWidth();
            rowCard->addChild(answer);
        }
    }

    auto homeButton = LFButton::create("Back Home");
    homeButton->matchParentWidth();
    homeButton->setHeight(56.0f);
    homeButton->setBorderRadius(22.0f);
    homeButton->setBorder(1.0f, 0xFF3567A3);
    homeButton->setFontSize(16.0f);
    homeButton->setTextColor(0xFFFFFFFF);
    homeButton->setBackgroundColor(LFButtonState::Normal, 0xFF3567A3);
    homeButton->setBackgroundColor(LFButtonState::Pressed, 0xFF2C598F);
    homeButton->setOnClick([weakSelf](LFButton*) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        self->goHome();
    });
    content->addChild(homeButton);
}

void PointPage::goHome() {
    if (auto navigator = getNavigator()) {
        while (navigator->getStackSize() > 1) {
            navigator->pop(false);
        }
    }
}
