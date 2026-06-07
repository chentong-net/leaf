#include "ExamPage.h"

#include "EnglishWordsI18n.h"
#include "PointPage.h"
#include "view/base/LFInput.h"

#include <algorithm>
#include <cctype>
#include <codecvt>
#include <cwctype>
#include <locale>
#include <string>
#include <utility>
#include <vector>

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

std::string trimCopy(const std::string& value) {
    size_t start = 0;
    size_t end = value.size();

    while (start < end && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return value.substr(start, end - start);
}

std::string collapseSpaces(const std::string& value) {
    std::string out;
    out.reserve(value.size());

    bool pendingSpace = false;
    for (char ch : value) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            pendingSpace = true;
            continue;
        }

        if (pendingSpace && !out.empty()) {
            out.push_back(' ');
        }
        pendingSpace = false;
        out.push_back(ch);
    }

    return out;
}

std::wstring utf8ToWide(const std::string& value) {
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    try {
        return converter.from_bytes(value);
    } catch (...) {
        return {};
    }
}

std::string wideToUtf8(const std::wstring& value) {
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    try {
        return converter.to_bytes(value);
    } catch (...) {
        return {};
    }
}

std::string toLowerUtf8(const std::string& value) {
    std::wstring wide = utf8ToWide(value);
    for (auto& ch : wide) {
        ch = static_cast<wchar_t>(std::towlower(static_cast<wint_t>(ch)));
    }
    return wideToUtf8(wide);
}

void replaceAll(std::string& value, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return;
    }

    size_t start = 0;
    while ((start = value.find(from, start)) != std::string::npos) {
        value.replace(start, from.size(), to);
        start += to.size();
    }
}

std::string normalizeToken(const std::string& value) {
    return toLowerUtf8(collapseSpaces(trimCopy(value)));
}

std::vector<std::string> splitNormalizedParts(std::string value, char delimiter) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= value.size()) {
        const size_t index = value.find(delimiter, start);
        std::string part = index == std::string::npos
            ? value.substr(start)
            : value.substr(start, index - start);
        part = normalizeToken(part);
        if (!part.empty()) {
            parts.push_back(part);
        }

        if (index == std::string::npos) {
            break;
        }
        start = index + 1;
    }

    return parts;
}

std::vector<std::string> expectedAnswerParts(const std::string& value) {
    std::string normalized = value;
    replaceAll(normalized, ";", ",");
    replaceAll(normalized, "/", ",");
    replaceAll(normalized, "，", ",");
    replaceAll(normalized, "；", ",");
    return splitNormalizedParts(normalized, ',');
}

std::vector<std::string> userAnswerParts(const std::string& value) {
    std::string normalized = value;
    replaceAll(normalized, "—", "-");
    replaceAll(normalized, "–", "-");
    replaceAll(normalized, "－", "-");
    replaceAll(normalized, ";", "-");
    replaceAll(normalized, ",", "-");
    replaceAll(normalized, "，", "-");
    replaceAll(normalized, "；", "-");
    replaceAll(normalized, "/", "-");
    return splitNormalizedParts(normalized, '-');
}

std::string answerPlaceholder(EnglishWordsTestMode mode) {
    switch (mode) {
        case EnglishWordsTestMode::ChineseToEnglish:
        case EnglishWordsTestMode::RussianToEnglish:
        case EnglishWordsTestMode::AudioToEnglish:
            return EnglishWordsI18n::tr("english_words.exam.enter_english_answer");
        case EnglishWordsTestMode::EnglishToChinese:
        case EnglishWordsTestMode::AudioToChinese:
            return EnglishWordsI18n::tr("english_words.exam.enter_chinese_answer");
        case EnglishWordsTestMode::EnglishToRussian:
        case EnglishWordsTestMode::AudioToRussian:
            return EnglishWordsI18n::tr("english_words.exam.enter_russian_answer");
    }

    return EnglishWordsI18n::tr("english_words.exam.enter_answer");
}

std::string questionPromptText(const EnglishWordEntry& entry, EnglishWordsTestMode mode) {
    switch (mode) {
        case EnglishWordsTestMode::ChineseToEnglish:
            return !entry.chineseTranslation.empty() ? entry.chineseTranslation : entry.text;
        case EnglishWordsTestMode::RussianToEnglish:
            return !entry.russianTranslation.empty() ? entry.russianTranslation : entry.text;
        case EnglishWordsTestMode::EnglishToChinese:
        case EnglishWordsTestMode::EnglishToRussian:
            return entry.text;
        case EnglishWordsTestMode::AudioToChinese:
        case EnglishWordsTestMode::AudioToRussian:
        case EnglishWordsTestMode::AudioToEnglish:
            return "";
    }

    return entry.text;
}

std::string expectedAnswerText(const EnglishWordEntry& entry, EnglishWordsTestMode mode) {
    switch (mode) {
        case EnglishWordsTestMode::ChineseToEnglish:
        case EnglishWordsTestMode::RussianToEnglish:
        case EnglishWordsTestMode::AudioToEnglish:
            return entry.text;
        case EnglishWordsTestMode::EnglishToChinese:
        case EnglishWordsTestMode::AudioToChinese:
            return !entry.chineseTranslation.empty() ? entry.chineseTranslation : entry.translation;
        case EnglishWordsTestMode::EnglishToRussian:
        case EnglishWordsTestMode::AudioToRussian:
            return !entry.russianTranslation.empty() ? entry.russianTranslation : entry.translation;
    }

    return entry.text;
}

bool isAudioMode(EnglishWordsTestMode mode) {
    return mode == EnglishWordsTestMode::AudioToChinese ||
           mode == EnglishWordsTestMode::AudioToRussian ||
           mode == EnglishWordsTestMode::AudioToEnglish;
}

double scoreAnswer(const EnglishWordEntry& entry,
                   EnglishWordsTestMode mode,
                   const std::string& userAnswer) {
    const auto expectedParts = expectedAnswerParts(expectedAnswerText(entry, mode));
    const auto userParts = userAnswerParts(userAnswer);
    if (expectedParts.empty() || userParts.empty()) {
        return 0.0;
    }

    int matchedCount = 0;
    for (const auto& expected : expectedParts) {
        if (std::find(userParts.begin(), userParts.end(), expected) != userParts.end()) {
            ++matchedCount;
        }
    }

    if (expectedParts.size() == 1) {
        return matchedCount == 1 ? 1.0 : 0.0;
    }
    if (matchedCount <= 0) {
        return 0.0;
    }
    if (matchedCount >= static_cast<int>(expectedParts.size())) {
        return 1.0;
    }
    return 0.5;
}

} // namespace

std::shared_ptr<ExamPage> ExamPage::create(const EnglishWordTopic& topic, EnglishWordsTestMode mode) {
    auto page = std::make_shared<ExamPage>();
    page->m_topic = topic;
    page->m_mode = mode;
    page->m_dataManager = EnglishWordsDataManager::create();
    page->setBackgroundColor(kPageBackgroundColor);
    page->buildUI();
    page->loadEntries();
    return page;
}

void ExamPage::onExit() {
    if (m_audioPlayer) {
        m_audioPlayer->stop();
    }
    LFPage::onExit();
}

void ExamPage::buildUI() {
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

    std::weak_ptr<ExamPage> weakSelf = std::static_pointer_cast<ExamPage>(shared_from_this());
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

    auto title = createText(m_topic.title, 21.0f, kTitleColor);
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
    content->setPadding(YGEdgeBottom, 20.0f);
    content->setSpacing(16.0f);
    scrollView->addChild(content);

    m_questionContainer = LFLinear::createVertical();
    m_questionContainer->matchParentWidth();
    m_questionContainer->wrapContentHeight();
    m_questionContainer->setPadding(YGEdgeAll, 20.0f);
    m_questionContainer->setSpacing(14.0f);
    m_questionContainer->setBackgroundColor(0xFFFFFFFF);
    m_questionContainer->setBorderRadius(28.0f);
    m_questionContainer->setBorder(1.0f, kCardBorderColor);
    m_questionContainer->setShadow(0.0f, 10.0f, 26.0f, 0.0f, kCardShadowColor);
    content->addChild(m_questionContainer);

    m_progressText = createText("", 13.0f, kSubtitleColor);
    m_progressText->matchParentWidth();
    m_progressText->setTextHAlign(LFTextHAlign::Center);
    m_questionContainer->addChild(m_progressText);

    m_promptText = createText("", 22.0f, kTitleColor);
    m_promptText->matchParentWidth();
    m_promptText->setTextHAlign(LFTextHAlign::Center);
    m_questionContainer->addChild(m_promptText);

    m_audioButton = LFButton::create(EnglishWordsI18n::tr("english_words.exam.play_audio"));
    m_audioButton->matchParentWidth();
    m_audioButton->setHeight(64.0f);
    m_audioButton->setBorderRadius(22.0f);
    m_audioButton->setBorder(1.0f, 0xFFD4E1EE);
    m_audioButton->setFontSize(17.0f);
    m_audioButton->setTextColor(0xFF1E3A5F);
    m_audioButton->setBackgroundColor(LFButtonState::Normal, 0xFFEAF3FF);
    m_audioButton->setBackgroundColor(LFButtonState::Pressed, 0xFFDCEBFF);
    m_audioButton->setOnClick([weakSelf](LFButton*) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        self->playCurrentAudio();
    });
    m_questionContainer->addChild(m_audioButton);

    m_answerHintText = createText(EnglishWordsI18n::tr("english_words.exam.answer_hint"), 12.0f, kSubtitleColor);
    m_answerHintText->matchParentWidth();
    m_questionContainer->addChild(m_answerHintText);

    m_answerInput = LFInput::create();
    m_answerInput->matchParentWidth();
    m_answerInput->setHeight(54.0f);
    m_answerInput->setBorderRadius(18.0f);
    m_answerInput->setBorder(1.0f, 0xFFD8E4F1);
    m_answerInput->setBackgroundColor(0xFFF8FBFE);
    m_answerInput->setFontSize(16.0f);
    m_answerInput->setTextColor(0xFF152033);
    m_answerInput->setPlaceholderColor(0xFF8898AA);
    m_answerInput->setOnChange([weakSelf](const std::string& text) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        if (self->m_currentIndex < 0 || self->m_currentIndex >= static_cast<int>(self->m_userAnswers.size())) {
            return;
        }
        self->m_userAnswers[static_cast<size_t>(self->m_currentIndex)] = text;
    });
    m_answerInput->setOnSubmit([weakSelf](const std::string&) {
        auto self = weakSelf.lock();
        if (!self || self->m_entries.empty()) {
            return;
        }
        if (self->m_currentIndex >= static_cast<int>(self->m_entries.size()) - 1) {
            self->submitExam();
        } else {
            self->goToIndex(self->m_currentIndex + 1);
        }
    });
    m_questionContainer->addChild(m_answerInput);

    m_actionRow = LFLinear::createHorizontal();
    m_actionRow->matchParentWidth();
    m_actionRow->wrapContentHeight();
    m_actionRow->setSpacing(12.0f);
    m_questionContainer->addChild(m_actionRow);

    m_previousButton = LFButton::create(EnglishWordsI18n::tr("english_words.exam.previous"));
    m_previousButton->setFlexGrow(1.0f);
    m_previousButton->setHeight(52.0f);
    m_previousButton->setBorderRadius(18.0f);
    m_previousButton->setBorder(1.0f, 0xFFD7E2EE);
    m_previousButton->setFontSize(15.0f);
    m_previousButton->setTextColor(0xFF1F3147);
    m_previousButton->setBackgroundColor(LFButtonState::Normal, 0xFFFFFFFF);
    m_previousButton->setBackgroundColor(LFButtonState::Pressed, 0xFFF3F7FB);
    m_previousButton->setOnClick([weakSelf](LFButton*) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        self->goToIndex(self->m_currentIndex - 1);
    });
    m_actionRow->addChild(m_previousButton);

    m_nextButton = LFButton::create(EnglishWordsI18n::tr("english_words.exam.next"));
    m_nextButton->setFlexGrow(1.0f);
    m_nextButton->setHeight(52.0f);
    m_nextButton->setBorderRadius(18.0f);
    m_nextButton->setBorder(1.0f, 0xFF3567A3);
    m_nextButton->setFontSize(15.0f);
    m_nextButton->setTextColor(0xFFFFFFFF);
    m_nextButton->setBackgroundColor(LFButtonState::Normal, 0xFF3567A3);
    m_nextButton->setBackgroundColor(LFButtonState::Pressed, 0xFF2C598F);
    m_nextButton->setOnClick([weakSelf](LFButton*) {
        auto self = weakSelf.lock();
        if (!self || self->m_entries.empty()) {
            return;
        }
        if (self->m_currentIndex >= static_cast<int>(self->m_entries.size()) - 1) {
            self->submitExam();
        } else {
            self->goToIndex(self->m_currentIndex + 1);
        }
    });
    m_actionRow->addChild(m_nextButton);
}

void ExamPage::loadEntries() {
    showStatus(EnglishWordsI18n::tr("english_words.exam.loading"));

    if (!m_dataManager) {
        showStatus(EnglishWordsI18n::tr("english_words.common.failed_load_words"));
        return;
    }

    std::weak_ptr<ExamPage> weakSelf = std::static_pointer_cast<ExamPage>(shared_from_this());
    m_dataManager->loadEntries(m_topic, [weakSelf](bool ok, std::vector<EnglishWordEntry> entries, const std::string&) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }

        if (!ok) {
            self->showStatus(EnglishWordsI18n::tr("english_words.common.failed_load_words"));
            return;
        }

        self->m_entries = std::move(entries);
        self->m_userAnswers.assign(self->m_entries.size(), "");
        self->m_currentIndex = 0;

        if (self->m_entries.empty()) {
            self->showStatus(EnglishWordsI18n::tr("english_words.common.no_words_available"));
            return;
        }

        self->refreshQuestion();
    });
}

void ExamPage::showStatus(const std::string& text) {
    m_statusMessage = text;

    if (!m_questionContainer) {
        return;
    }

    if (m_progressText) {
        m_progressText->setDisplay(YGDisplayNone);
    }
    if (m_promptText) {
        m_promptText->setDisplay(YGDisplayFlex);
        m_promptText->setText(text);
        m_promptText->setTextHAlign(LFTextHAlign::Center);
    }
    if (m_audioButton) {
        m_audioButton->setDisplay(YGDisplayNone);
    }
    if (m_answerHintText) {
        m_answerHintText->setDisplay(YGDisplayNone);
    }
    if (m_answerInput) {
        m_answerInput->setDisplay(YGDisplayNone);
    }
    if (m_actionRow) {
        m_actionRow->setDisplay(YGDisplayNone);
    }
}

void ExamPage::refreshQuestion() {
    if (m_entries.empty() || m_currentIndex < 0 || m_currentIndex >= static_cast<int>(m_entries.size())) {
        showStatus(EnglishWordsI18n::tr("english_words.common.no_words_available"));
        return;
    }

    const auto& entry = m_entries[static_cast<size_t>(m_currentIndex)];
    const bool audioMode = isAudioMode(m_mode);

    if (m_progressText) {
        m_progressText->setDisplay(YGDisplayFlex);
        m_progressText->setText(EnglishWordsI18n::questionProgress(
            m_currentIndex + 1,
            static_cast<int>(m_entries.size())
        ));
    }

    if (m_promptText) {
        m_promptText->setText(questionPromptText(entry, m_mode));
        m_promptText->setTextHAlign(LFTextHAlign::Center);
        m_promptText->setDisplay(audioMode ? YGDisplayNone : YGDisplayFlex);
    }

    if (m_audioButton) {
        m_audioButton->setDisplay(audioMode ? YGDisplayFlex : YGDisplayNone);
        m_audioButton->setEnabled(!entry.audioAssetPath.empty());
        m_audioButton->setText(entry.audioAssetPath.empty()
            ? EnglishWordsI18n::tr("english_words.exam.audio_unavailable")
            : EnglishWordsI18n::tr("english_words.exam.play_audio"));
    }

    if (m_answerHintText) {
        m_answerHintText->setDisplay(YGDisplayFlex);
    }

    if (m_answerInput) {
        m_answerInput->setDisplay(YGDisplayFlex);
        m_answerInput->setPlaceholder(answerPlaceholder(m_mode));
        m_answerInput->setText(m_userAnswers[static_cast<size_t>(m_currentIndex)]);
    }

    if (m_actionRow) {
        m_actionRow->setDisplay(YGDisplayFlex);
    }
    if (m_previousButton) {
        m_previousButton->setDisplay(m_currentIndex > 0 ? YGDisplayFlex : YGDisplayNone);
    }
    if (m_nextButton) {
        m_nextButton->setDisplay(YGDisplayFlex);
        m_nextButton->setText(m_currentIndex >= static_cast<int>(m_entries.size()) - 1
            ? EnglishWordsI18n::tr("english_words.exam.submit")
            : EnglishWordsI18n::tr("english_words.exam.next"));
    }
}

void ExamPage::playCurrentAudio() {
    if (m_currentIndex < 0 || m_currentIndex >= static_cast<int>(m_entries.size()) || !m_dataManager) {
        return;
    }

    const auto& entry = m_entries[static_cast<size_t>(m_currentIndex)];
    if (entry.audioAssetPath.empty()) {
        return;
    }

    if (!m_audioPlayer) {
        m_audioPlayer = LFAudioPlayer::create();
    }

    std::weak_ptr<ExamPage> weakSelf = std::static_pointer_cast<ExamPage>(shared_from_this());
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

void ExamPage::goToIndex(int index) {
    if (index < 0 || index >= static_cast<int>(m_entries.size())) {
        return;
    }

    m_currentIndex = index;
    refreshQuestion();
}

void ExamPage::submitExam() {
    if (m_entries.empty() || !m_dataManager) {
        return;
    }

    EnglishWordsExamResult result;
    result.topic = m_topic;
    result.mode = m_mode;
    result.questionCount = static_cast<int>(m_entries.size());
    result.questions.reserve(m_entries.size());

    for (size_t index = 0; index < m_entries.size(); ++index) {
        EnglishWordsQuestionResult question;
        question.entry = m_entries[index];
        question.mode = m_mode;
        question.userAnswer = index < m_userAnswers.size() ? m_userAnswers[index] : "";
        question.score = scoreAnswer(question.entry, m_mode, question.userAnswer);
        result.totalScore += question.score;
        result.questions.push_back(std::move(question));
    }

    const EnglishWordsExamResult fallbackResult = result;
    std::weak_ptr<ExamPage> weakSelf = std::static_pointer_cast<ExamPage>(shared_from_this());
    m_dataManager->saveExamResult(std::move(result), [weakSelf, fallbackResult](bool ok,
                                                                               EnglishWordsExamResult savedResult,
                                                                               const std::string&) mutable {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }

        if (auto navigator = self->getNavigator()) {
            navigator->replace(PointPage::create(ok ? savedResult : fallbackResult));
        }
    });
}
