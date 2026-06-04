#include "MainPage.h"

#include "EnglishWordsUI.h"

#include <array>
#include <string>
#include <utility>

namespace {

struct ShortcutSpec {
    const char* title;
    const char* subtitle;
    const char* iconPath;
    uint32_t iconSurfaceColor;
    uint32_t pressedColor;
    bool opensStudyPage;
};

const std::array<ShortcutSpec, 6> kShortcuts = {{
    {"Study", "Vocabulary learning and daily drills.", "EnglishWordsAssets/Images/icon-study.png", 0xFFEAF3FF, 0xFFF2F7FF, true},
    {"Quick Test", "Fast practice with timed questions.", "EnglishWordsAssets/Images/icon-test.png", 0xFFFFF1D6, 0xFFFFF6E6, false},
    {"Test Setup", "Choose range, mode, and rules.", "EnglishWordsAssets/Images/icon-settings.png", 0xFFF1F5F9, 0xFFF5F8FC, false},
    {"Results", "Review scores and recent sessions.", "EnglishWordsAssets/Images/icon-result.png", 0xFFE6F4EA, 0xFFEEF8F1, false},
    {"My Words", "Keep collected words in one place.", "EnglishWordsAssets/Images/icon-collect.png", 0xFFFFECEB, 0xFFFFF3F2, false},
    {"Help", "Read usage tips and key guidance.", "EnglishWordsAssets/Images/icon-help.png", 0xFFE6FFFB, 0xFFEEFFFC, false},
}};

LFButton::Ptr createHeaderButton(const std::string& iconPath, std::function<void()> onTap) {
    auto button = LFButton::create();
    button->setWidth(52.0f);
    button->setHeight(52.0f);
    button->setBorderRadius(18.0f);
    button->setBorder(1.0f, 0xFFD8E4F1);
    button->setBackgroundColor(LFButtonState::Normal, 0xFFFFFFFF);
    button->setBackgroundColor(LFButtonState::Pressed, 0xFFEAF3FF);
    button->setShadow(0.0f, 8.0f, 20.0f, 0.0f, 0x10233B53);
    button->addChild(EnglishWordsUI::makeImage(iconPath, 22.0f), LFBoxAlign::Center);
    button->setOnClick([onTap = std::move(onTap)](LFButton*) {
        if (onTap) {
            onTap();
        }
    });
    return button;
}

LFButton::Ptr createShortcutCard(const ShortcutSpec& spec, std::function<void()> onTap) {
    auto button = LFButton::create();
    button->setHeight(154.0f);
    button->setBorderRadius(22.0f);
    button->setBorder(1.0f, EnglishWordsUI::kCardBorderColor);
    button->setBackgroundColor(LFButtonState::Normal, 0xFFFFFFFF);
    button->setBackgroundColor(LFButtonState::Pressed, spec.pressedColor);
    button->setShadow(0.0f, 10.0f, 24.0f, 0.0f, EnglishWordsUI::kCardShadowColor);

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
    iconBubble->addChild(EnglishWordsUI::makeImage(spec.iconPath, 24.0f), LFBoxAlign::Center);
    topRow->addChild(iconBubble);

    auto arrow = EnglishWordsUI::makeImage("arrow-right.png", 14.0f);
    arrow->setOpacity(0.58f);
    topRow->addChild(arrow);
    content->addChild(topRow);

    auto bottom = LFLinear::createVertical();
    bottom->matchParentWidth();
    bottom->wrapContentHeight();
    bottom->setSpacing(6.0f);
    bottom->addChild(EnglishWordsUI::makeText(spec.title, 17.0f, EnglishWordsUI::kTitleColor));
    bottom->addChild(EnglishWordsUI::makeText(spec.subtitle, 12.5f, 0xFF6B7A90));
    content->addChild(bottom);

    button->setOnClick([onTap = std::move(onTap)](LFButton*) {
        if (onTap) {
            onTap();
        }
    });
    return button;
}

} // namespace

std::shared_ptr<MainPage> MainPage::create(std::function<void()> onStudyTap) {
    auto page = std::make_shared<MainPage>();
    page->m_onStudyTap = std::move(onStudyTap);
    page->setBackgroundColor(EnglishWordsUI::kPageBackgroundColor);
    page->buildUI();
    return page;
}

void MainPage::buildUI() {
    auto scrollView = LFScrollView::createVertical();
    scrollView->matchParentWidth();
    scrollView->matchParentHeight();
    scrollView->setBounces(false);
    addChild(scrollView);

    auto content = LFLinear::createVertical();
    content->matchParentWidth();
    content->wrapContentHeight();
    content->setPadding(YGEdgeAll, 20.0f);
    content->setSpacing(18.0f);
    content->setBackgroundColor(EnglishWordsUI::kPageBackgroundColor);
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
    headerLeft->addChild(EnglishWordsUI::makeText("EnglishWords", 30.0f, EnglishWordsUI::kTitleColor));
    headerLeft->addChild(EnglishWordsUI::makeText("English word trainer built with Leaf.", 13.0f, 0xFF6B7A90));
    headerRow->addChild(headerLeft);

    m_statusText = EnglishWordsUI::makeText("Selected: none", 13.0f, 0xFFF8FBFF);
    m_statusText->matchParentWidth();
    m_statusText->setMaxLines(1);

    headerRow->addChild(createHeaderButton("EnglishWordsAssets/Images/icon-settings.png", [this]() {
        updateStatus("Selected: Settings");
    }));
    content->addChild(headerRow);

    auto heroCard = LFLinear::createVertical();
    heroCard->matchParentWidth();
    heroCard->wrapContentHeight();
    heroCard->setPadding(YGEdgeAll, 20.0f);
    heroCard->setSpacing(10.0f);
    heroCard->setBackgroundColor(0xFF3567A3);
    heroCard->setBorderRadius(28.0f);
    heroCard->setShadow(0.0f, 14.0f, 30.0f, 0.0f, 0x183567A3);
    heroCard->addChild(EnglishWordsUI::makeText("Home preview", 21.0f, 0xFFFFFFFF));
    heroCard->addChild(EnglishWordsUI::makeText("The page is UI-first for now. Tap any entry to verify layout and click handling.", 13.0f, 0xFFDDEBFF));

    auto statusBadge = LFLinear::createHorizontal();
    statusBadge->matchParentWidth();
    statusBadge->wrapContentHeight();
    statusBadge->setPadding(YGEdgeLeft, 12.0f);
    statusBadge->setPadding(YGEdgeRight, 12.0f);
    statusBadge->setPadding(YGEdgeTop, 10.0f);
    statusBadge->setPadding(YGEdgeBottom, 10.0f);
    statusBadge->setBackgroundColor(0x1FFFFFFF);
    statusBadge->setBorderRadius(16.0f);
    statusBadge->addChild(m_statusText);
    heroCard->addChild(statusBadge);
    content->addChild(heroCard);

    auto sectionLabel = EnglishWordsUI::makeText("Quick Access", 14.0f, 0xFF516174);
    sectionLabel->matchParentWidth();
    content->addChild(sectionLabel);

    auto gridSurface = LFLinear::createVertical();
    gridSurface->matchParentWidth();
    gridSurface->wrapContentHeight();
    gridSurface->setPadding(YGEdgeAll, 8.0f);
    gridSurface->setBackgroundColor(EnglishWordsUI::kSurfaceColor);
    gridSurface->setBorderRadius(28.0f);
    gridSurface->setBorder(1.0f, EnglishWordsUI::kSurfaceBorderColor);

    auto grid = LFGrid::create(2, 10.0f);
    grid->matchParentWidth();
    grid->wrapContentHeight();
    for (const auto& shortcut : kShortcuts) {
        const std::string title = shortcut.title;
        grid->addChild(createShortcutCard(shortcut, [this, title, opensStudyPage = shortcut.opensStudyPage]() {
            if (opensStudyPage) {
                if (m_onStudyTap) {
                    m_onStudyTap();
                }
                return;
            }
            updateStatus(std::string("Selected: ") + title);
        }));
    }

    gridSurface->addChild(grid);
    content->addChild(gridSurface);

    auto footnote = EnglishWordsUI::makeText("Navigation is intentionally deferred. This page is for home UI validation on desktop and mobile layouts.", 12.0f, 0xFF7A889C);
    footnote->matchParentWidth();
    content->addChild(footnote);
}

void MainPage::updateStatus(const std::string& text) {
    if (m_statusText) {
        m_statusText->setText(text);
    }
}
