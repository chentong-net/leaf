#include "MainPage.h"

#include "LearningPage.h"

#include <array>
#include <string>

namespace {

constexpr uint32_t kPageBackgroundColor = 0xFFF4F7FB;

struct ShortcutSpec {
    const char* title;
    const char* iconPath;
    uint32_t iconSurfaceColor;
    uint32_t pressedColor;
    bool opensStudyPage;
};

const std::array<ShortcutSpec, 6> kShortcuts = {{
    {"Study", "EnglishWordsAssets/Images/icon-study.png", 0xFFEAF3FF, 0xFFF2F7FF, true},
    {"Quick Test", "EnglishWordsAssets/Images/icon-test.png", 0xFFFFF1D6, 0xFFFFF6E6, false},
    {"Test Setup", "EnglishWordsAssets/Images/icon-settings.png", 0xFFF1F5F9, 0xFFF5F8FC, false},
    {"Results", "EnglishWordsAssets/Images/icon-result.png", 0xFFE6F4EA, 0xFFEEF8F1, false},
    {"My Words", "EnglishWordsAssets/Images/icon-collect.png", 0xFFFFECEB, 0xFFFFF3F2, false},
    {"Help", "EnglishWordsAssets/Images/icon-help.png", 0xFFE6FFFB, 0xFFEEFFFC, false},
}};

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

LFButton::Ptr createHeaderButton(const std::string& iconPath, std::function<void()> onClick) {
    auto button = LFButton::create();
    button->setWidth(52.0f);
    button->setHeight(52.0f);
    button->setBorderRadius(18.0f);
    button->setBorder(1.0f, 0xFFD8E4F1);
    button->setBackgroundColor(LFButtonState::Normal, 0xFFFFFFFF);
    button->setBackgroundColor(LFButtonState::Pressed, 0xFFEAF3FF);
    button->setShadow(0.0f, 8.0f, 20.0f, 0.0f, 0x10233B53);
    button->addChild(createImage(iconPath, 22.0f), LFBoxAlign::Center);
    button->setOnClick([onClick = std::move(onClick)](LFButton*) {
        if (onClick) {
            onClick();
        }
    });
    return button;
}

} // namespace

std::shared_ptr<MainPage> MainPage::create() {
    auto page = std::make_shared<MainPage>();
    page->setBackgroundColor(kPageBackgroundColor);
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
    content->setBackgroundColor(kPageBackgroundColor);
    scrollView->addChild(content);

    auto headerRow = LFLinear::createHorizontal();
    headerRow->matchParentWidth();
    headerRow->wrapContentHeight();
    headerRow->setAlignItems(YGAlignCenter);
    headerRow->setSpacing(12.0f);
    content->addChild(headerRow);

    auto headerLeft = LFLinear::createVertical();
    headerLeft->setFlexGrow(1.0f);
    headerLeft->wrapContentHeight();
    headerRow->addChild(headerLeft);

    auto appName = createText("EnglishWords", 30.0f, 0xFF142033);
    appName->matchParentWidth();
    headerLeft->addChild(appName);

    headerRow->addChild(createHeaderButton("EnglishWordsAssets/Images/icon-settings.png", {}));

    auto heroCard = LFLinear::createVertical();
    heroCard->matchParentWidth();
    heroCard->wrapContentHeight();
    heroCard->setPadding(YGEdgeAll, 20.0f);
    heroCard->setSpacing(8.0f);
    heroCard->setBackgroundColor(0xFF3567A3);
    heroCard->setBorderRadius(28.0f);
    heroCard->setShadow(0.0f, 14.0f, 30.0f, 0.0f, 0x183567A3);
    content->addChild(heroCard);

    auto heroTitle = createText("Home", 21.0f, 0xFFFFFFFF);
    heroTitle->matchParentWidth();
    heroCard->addChild(heroTitle);

    auto heroSubtitle = createText("Study English words by topic and level.", 13.0f, 0xFFDDEBFF);
    heroSubtitle->matchParentWidth();
    heroSubtitle->setMaxLines(1);
    heroCard->addChild(heroSubtitle);

    auto heroAction = createText("Choose a module and continue today.", 13.0f, 0xFFFFFFFF);
    heroAction->matchParentWidth();
    heroAction->setMaxLines(1);
    heroCard->addChild(heroAction);

    auto sectionLabel = createText("Quick Access", 14.0f, 0xFF516174);
    sectionLabel->matchParentWidth();
    content->addChild(sectionLabel);

    auto gridSurface = LFLinear::createVertical();
    gridSurface->matchParentWidth();
    gridSurface->wrapContentHeight();
    gridSurface->setPadding(YGEdgeAll, 8.0f);
    gridSurface->setBackgroundColor(0xFFEAF1F8);
    gridSurface->setBorderRadius(28.0f);
    gridSurface->setBorder(1.0f, 0xFFDDE7F1);
    content->addChild(gridSurface);

    auto grid = LFGrid::create(2, 10.0f);
    grid->matchParentWidth();
    grid->wrapContentHeight();
    gridSurface->addChild(grid);

    std::weak_ptr<MainPage> weakSelf = std::static_pointer_cast<MainPage>(shared_from_this());

    for (const auto& shortcut : kShortcuts) {
        auto button = LFButton::create();
        button->setHeight(100.0f);
        button->setBorderRadius(22.0f);
        button->setBorder(1.0f, 0xFFDCE6F2);
        button->setBackgroundColor(LFButtonState::Normal, 0xFFFFFFFF);
        button->setBackgroundColor(LFButtonState::Pressed, shortcut.pressedColor);
        button->setShadow(0.0f, 10.0f, 24.0f, 0.0f, 0x12233B53);
        grid->addChild(button);

        auto vl = LFLinear::createVertical();
        vl->matchParentWidth();
        vl->wrapContentHeight();
        vl->setSpacing(12);
        vl->setGravity(LFAlignment::Center, LFAlignment::Center);
        vl->setPadding(YGEdgeVertical, 4);

        auto iconBubble = LFBox::create();
        iconBubble->setWidth(50.0f);
        iconBubble->setHeight(50.0f);
        iconBubble->setBorderRadius(17.0f);
        iconBubble->setBackgroundColor(shortcut.iconSurfaceColor);
        iconBubble->setBorder(1.0f, 0x0DE2E8F0);
        iconBubble->addChild(createImage(shortcut.iconPath, 24.0f), LFBoxAlign::Center);
        vl->addChild(iconBubble);

        auto title = createText(shortcut.title, 17.0f, 0xFF142033);
        title->matchParentWidth();
        title->setTextHAlign(LFTextHAlign::Center);
        title->setMaxLines(1);
        vl->addChild(title);

        button->addChild(vl, LFBoxAlign::Center);

        button->setOnClick([weakSelf, opensStudyPage = shortcut.opensStudyPage](LFButton*) {
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }

            if (opensStudyPage) {
                if (auto navigator = self->getNavigator()) {
                    navigator->push(LearningPage::create());
                }
                return;
            }
        });
    }
}
