#include "MainPage.h"

#include "LearningPage.h"

#include <array>
#include <string>

namespace {

struct ShortcutSpec {
    const char* title;
    const char* subtitle;
    const char* iconPath;
    uint32_t iconSurfaceColor;
    uint32_t pressedColor;
    bool opensStudyPage;
};

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

LFButton::Ptr makeHeaderIconButton(const std::shared_ptr<LFText>& statusText) {
    auto button = LFButton::create();
    button->setWidth(52.0f);
    button->setHeight(52.0f);
    button->setBorderRadius(18.0f);
    button->setBorder(1.0f, 0xFFD8E4F1);
    button->setBackgroundColor(LFButtonState::Normal, 0xFFFFFFFF);
    button->setBackgroundColor(LFButtonState::Pressed, 0xFFEAF3FF);
    button->setShadow(0.0f, 8.0f, 20.0f, 0.0f, 0x10233B53);

    auto icon = makeImage("EnglishWordsAssets/Images/icon-settings.png", 22.0f);
    button->addChild(icon, LFBoxAlign::Center);

    button->setOnClick([statusText](LFButton*) {
        if (statusText) {
            statusText->setText("Selected: Settings");
        }
    });

    return button;
}

LFButton::Ptr makeShortcutCard(const ShortcutSpec& spec,
                               const std::shared_ptr<LFText>& statusText,
                               std::weak_ptr<LFNavigator> nav) {
    auto button = LFButton::create();
    button->setHeight(154.0f);
    button->setBorderRadius(22.0f);
    button->setBorder(1.0f, 0xFFDCE6F2);
    button->setBackgroundColor(LFButtonState::Normal, 0xFFFFFFFF);
    button->setBackgroundColor(LFButtonState::Pressed, spec.pressedColor);
    button->setShadow(0.0f, 10.0f, 24.0f, 0.0f, 0x12233B53);

    auto content = LFLinear::createVertical();
    content->matchParentWidth();
    content->matchParentHeight();
    content->setPadding(YGEdgeAll, 16.0f);
    content->setJustifyContent(YGJustifySpaceBetween);
    button->addChild(content, LFBoxAlign::MatchParent);

    auto topRow = LFLinear::createHorizontal();
    topRow->matchParentWidth();
    topRow->wrapContentHeight();
    topRow->setAlignItems(YGAlignCenter);
    topRow->setJustifyContent(YGJustifySpaceBetween);

    auto iconBubble = LFBox::create();
    iconBubble->setWidth(50.0f);
    iconBubble->setHeight(50.0f);
    iconBubble->setBorderRadius(17.0f);
    iconBubble->setBackgroundColor(spec.iconSurfaceColor);
    iconBubble->setBorder(1.0f, 0x0DE2E8F0);
    iconBubble->addChild(makeImage(spec.iconPath, 24.0f), LFBoxAlign::Center);
    topRow->addChild(iconBubble);

    auto arrow = makeImage("arrow-right.png", 14.0f);
    arrow->setOpacity(0.58f);
    topRow->addChild(arrow);

    content->addChild(topRow);

    auto bottom = LFLinear::createVertical();
    bottom->matchParentWidth();
    bottom->wrapContentHeight();
    bottom->setSpacing(6.0f);

    auto title = makeText(spec.title, 17.0f, 0xFF142033);
    title->matchParentWidth();
    bottom->addChild(title);

    auto subtitle = makeText(spec.subtitle, 12.5f, 0xFF6B7A90);
    subtitle->matchParentWidth();
    bottom->addChild(subtitle);

    content->addChild(bottom);

    const std::string titleText = spec.title;
    button->setOnClick([statusText, titleText, opensStudyPage = spec.opensStudyPage, nav](LFButton*) {
        if (opensStudyPage) {
            if (auto navigator = nav.lock()) {
                navigator->push(LearningPage::create(navigator));
                return;
            }
        }
        if (statusText) {
            statusText->setText("Selected: " + titleText);
        }
    });

    return button;
}

}

LFPage::Ptr MainPage::create(std::weak_ptr<LFNavigator> nav) {
    static const std::array<ShortcutSpec, 6> kShortcuts = {{
        {"Study", "Vocabulary learning and daily drills.", "EnglishWordsAssets/Images/icon-study.png", 0xFFEAF3FF, 0xFFF2F7FF, true},
        {"Quick Test", "Fast practice with timed questions.", "EnglishWordsAssets/Images/icon-test.png", 0xFFFFF1D6, 0xFFFFF6E6, false},
        {"Test Setup", "Choose range, mode, and rules.", "EnglishWordsAssets/Images/icon-settings.png", 0xFFF1F5F9, 0xFFF5F8FC, false},
        {"Results", "Review scores and recent sessions.", "EnglishWordsAssets/Images/icon-result.png", 0xFFE6F4EA, 0xFFEEF8F1, false},
        {"My Words", "Keep collected words in one place.", "EnglishWordsAssets/Images/icon-collect.png", 0xFFFFECEB, 0xFFFFF3F2, false},
        {"Help", "Read usage tips and key guidance.", "EnglishWordsAssets/Images/icon-help.png", 0xFFE6FFFB, 0xFFEEFFFC, false},
    }};

    auto page = LFPage::create();
    page->setBackgroundColor(0xFFF4F7FB);

    auto scrollView = LFScrollView::createVertical();
    scrollView->matchParentWidth();
    scrollView->matchParentHeight();
    scrollView->setBounces(false);
    page->addChild(scrollView);

    auto content = LFLinear::createVertical();
    content->matchParentWidth();
    content->wrapContentHeight();
    content->setPadding(YGEdgeAll, 20.0f);
    content->setSpacing(18.0f);
    content->setBackgroundColor(0xFFF4F7FB);
    scrollView->addChild(content);

    auto headerRow = LFLinear::createHorizontal();
    headerRow->matchParentWidth();
    headerRow->wrapContentHeight();
    headerRow->setAlignItems(YGAlignCenter);
    headerRow->setSpacing(12.0f);

    auto headerLeft = LFLinear::createVertical();
    headerLeft->setFlexGrow(1.0f);
    headerLeft->wrapContentHeight();
    headerLeft->setSpacing(4.0f);

    auto appName = makeText("EnglishWords", 30.0f, 0xFF142033);
    appName->matchParentWidth();
    headerLeft->addChild(appName);

    auto subtitle = makeText("English word trainer built with Leaf.", 13.0f, 0xFF6B7A90);
    subtitle->matchParentWidth();
    headerLeft->addChild(subtitle);

    headerRow->addChild(headerLeft);

    auto statusText = makeText("Selected: none", 13.0f, 0xFFF8FBFF);
    statusText->matchParentWidth();
    statusText->setMaxLines(1);

    headerRow->addChild(makeHeaderIconButton(statusText));
    content->addChild(headerRow);

    auto heroCard = LFLinear::createVertical();
    heroCard->matchParentWidth();
    heroCard->wrapContentHeight();
    heroCard->setPadding(YGEdgeAll, 20.0f);
    heroCard->setSpacing(10.0f);
    heroCard->setBackgroundColor(0xFF3567A3);
    heroCard->setBorderRadius(28.0f);
    heroCard->setShadow(0.0f, 14.0f, 30.0f, 0.0f, 0x183567A3);

    auto heroTitle = makeText("Home preview", 21.0f, 0xFFFFFFFF);
    heroTitle->matchParentWidth();
    heroCard->addChild(heroTitle);

    auto heroSubtitle = makeText("The page is UI-first for now. Tap any entry to verify layout and click handling.", 13.0f, 0xFFDDEBFF);
    heroSubtitle->matchParentWidth();
    heroCard->addChild(heroSubtitle);

    auto statusBadge = LFLinear::createHorizontal();
    statusBadge->matchParentWidth();
    statusBadge->wrapContentHeight();
    statusBadge->setPadding(YGEdgeLeft, 12.0f);
    statusBadge->setPadding(YGEdgeRight, 12.0f);
    statusBadge->setPadding(YGEdgeTop, 10.0f);
    statusBadge->setPadding(YGEdgeBottom, 10.0f);
    statusBadge->setBackgroundColor(0x1FFFFFFF);
    statusBadge->setBorderRadius(16.0f);
    statusBadge->addChild(statusText);
    heroCard->addChild(statusBadge);

    content->addChild(heroCard);

    auto sectionLabel = makeText("Quick Access", 14.0f, 0xFF516174);
    sectionLabel->matchParentWidth();
    content->addChild(sectionLabel);

    auto gridSurface = LFLinear::createVertical();
    gridSurface->matchParentWidth();
    gridSurface->wrapContentHeight();
    gridSurface->setPadding(YGEdgeAll, 8.0f);
    gridSurface->setBackgroundColor(0xFFEAF1F8);
    gridSurface->setBorderRadius(28.0f);
    gridSurface->setBorder(1.0f, 0xFFDDE7F1);

    auto grid = LFGrid::create(2, 10.0f);
    grid->matchParentWidth();
    grid->wrapContentHeight();

    for (const auto& shortcut : kShortcuts) {
        grid->addChild(makeShortcutCard(shortcut, statusText, nav));
    }

    gridSurface->addChild(grid);
    content->addChild(gridSurface);

    auto footnote = makeText("Navigation is intentionally deferred. This page is for home UI validation on desktop and mobile layouts.", 12.0f, 0xFF7A889C);
    footnote->matchParentWidth();
    content->addChild(footnote);

    return page;
}
