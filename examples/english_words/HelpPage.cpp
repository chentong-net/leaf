#include "HelpPage.h"

#include "EnglishWordsI18n.h"

#include <array>
#include <string>

namespace {

constexpr uint32_t kPageBackgroundColor = 0xFFF4F7FB;
constexpr uint32_t kTitleColor = 0xFF142033;
constexpr uint32_t kSubtitleColor = 0xFF6B7A90;
constexpr uint32_t kCardBorderColor = 0xFFDCE6F2;
constexpr uint32_t kCardShadowColor = 0x12233B53;

struct HelpSectionSpec {
    const char* iconPath;
    uint32_t tintColor;
    const char* titleKey;
    const char* bodyKey;
};

const std::array<HelpSectionSpec, 4> kHelpSections = {{
    {"EnglishWordsAssets/Images/icon-study.png", 0xFFEAF3FF, "english_words.help.study_title", "english_words.help.study_body"},
    {"EnglishWordsAssets/Images/icon-test.png", 0xFFFFF4DE, "english_words.help.test_title", "english_words.help.test_body"},
    {"EnglishWordsAssets/Images/icon-result.png", 0xFFE9F7F0, "english_words.help.results_title", "english_words.help.results_body"},
    {"EnglishWordsAssets/Images/icon-settings.png", 0xFFEEF1FF, "english_words.help.settings_title", "english_words.help.settings_body"},
}};

std::shared_ptr<LFText> createText(const std::string& text, float size, uint32_t color) {
    auto node = std::make_shared<LFText>();
    node->setText(text);
    node->setFontSize(size);
    node->setTextColor(color);
    node->setLineHeight(1.35f);
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

LFLinear::Ptr createSectionCard(const HelpSectionSpec& section) {
    auto card = LFLinear::createVertical();
    card->matchParentWidth();
    card->wrapContentHeight();
    card->setPadding(YGEdgeAll, 18.0f);
    card->setSpacing(12.0f);
    card->setBackgroundColor(0xFFFFFFFF);
    card->setBorderRadius(24.0f);
    card->setBorder(1.0f, kCardBorderColor);
    card->setShadow(0.0f, 10.0f, 24.0f, 0.0f, kCardShadowColor);

    auto header = LFLinear::createHorizontal();
    header->matchParentWidth();
    header->wrapContentHeight();
    header->setSpacing(12.0f);
    header->setAlignItems(YGAlignCenter);
    card->addChild(header);

    auto iconBubble = LFBox::create();
    iconBubble->setWidth(46.0f);
    iconBubble->setHeight(46.0f);
    iconBubble->setBorderRadius(16.0f);
    iconBubble->setBackgroundColor(section.tintColor);
    iconBubble->addChild(createImage(section.iconPath, 22.0f), LFBoxAlign::Center);
    header->addChild(iconBubble);

    auto title = createText(EnglishWordsI18n::tr(section.titleKey), 17.0f, kTitleColor);
    title->setFlexGrow(1.0f);
    title->setFlexBasis(0.0f);
    title->setMaxLines(2);
    header->addChild(title);

    auto body = createText(EnglishWordsI18n::tr(section.bodyKey), 13.5f, kSubtitleColor);
    body->matchParentWidth();
    card->addChild(body);

    return card;
}

} // namespace

std::shared_ptr<HelpPage> HelpPage::create() {
    auto page = std::make_shared<HelpPage>();
    page->setBackgroundColor(kPageBackgroundColor);
    page->buildUI();
    return page;
}

void HelpPage::buildUI() {
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

    std::weak_ptr<HelpPage> weakSelf = std::static_pointer_cast<HelpPage>(shared_from_this());
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

    auto title = createText(EnglishWordsI18n::tr("english_words.help.title"), 22.0f, kTitleColor);
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
    content->setSpacing(14.0f);
    scrollView->addChild(content);

    auto introCard = LFLinear::createVertical();
    introCard->matchParentWidth();
    introCard->wrapContentHeight();
    introCard->setPadding(YGEdgeAll, 20.0f);
    introCard->setSpacing(10.0f);
    introCard->setBackgroundColor(0xFF3567A3);
    introCard->setBorderRadius(28.0f);
    introCard->setShadow(0.0f, 14.0f, 30.0f, 0.0f, 0x183567A3);
    content->addChild(introCard);

    auto introTitle = createText(EnglishWordsI18n::tr("english_words.help.intro_title"), 19.0f, 0xFFFFFFFF);
    introTitle->matchParentWidth();
    introCard->addChild(introTitle);

    auto introBody = createText(EnglishWordsI18n::tr("english_words.help.intro_body"), 13.5f, 0xFFDDEBFF);
    introBody->matchParentWidth();
    introCard->addChild(introBody);

    for (const auto& section : kHelpSections) {
        content->addChild(createSectionCard(section));
    }

    auto tipCard = LFLinear::createVertical();
    tipCard->matchParentWidth();
    tipCard->wrapContentHeight();
    tipCard->setPadding(YGEdgeAll, 18.0f);
    tipCard->setSpacing(8.0f);
    tipCard->setBackgroundColor(0xFFEAF1F8);
    tipCard->setBorderRadius(24.0f);
    tipCard->setBorder(1.0f, 0xFFDDE7F1);
    content->addChild(tipCard);

    auto tipTitle = createText(EnglishWordsI18n::tr("english_words.help.tip_title"), 16.0f, kTitleColor);
    tipTitle->matchParentWidth();
    tipCard->addChild(tipTitle);

    auto tipBody = createText(EnglishWordsI18n::tr("english_words.help.tip_body"), 13.0f, kSubtitleColor);
    tipBody->matchParentWidth();
    tipCard->addChild(tipBody);
}
