#include "SettingsPage.h"

#include "EnglishWordsI18n.h"
#include "LFI18n.h"
#include "view/wrapped/LFDropdown.h"

#include <array>
#include <string>

namespace {

constexpr uint32_t kPageBackgroundColor = 0xFFF4F7FB;
constexpr uint32_t kTitleColor = 0xFF142033;
constexpr uint32_t kSubtitleColor = 0xFF6B7A90;
constexpr uint32_t kCardBorderColor = 0xFFDCE6F2;
constexpr uint32_t kCardShadowColor = 0x12233B53;

struct LanguageOption {
    const char* tag;
    const char* label;
};

const std::array<LanguageOption, 3> kLanguageOptions = {{
    {"zh-CN", "简体中文"},
    {"en-US", "English"},
    {"ru-RU", "Русский"},
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

std::vector<std::string> languageLabels() {
    std::vector<std::string> labels;
    labels.reserve(kLanguageOptions.size());
    for (const auto& option : kLanguageOptions) {
        labels.emplace_back(option.label);
    }
    return labels;
}

} // namespace

std::shared_ptr<SettingsPage> SettingsPage::create() {
    auto page = std::make_shared<SettingsPage>();
    page->setBackgroundColor(kPageBackgroundColor);
    page->buildOverlay();
    page->buildUI();
    return page;
}

void SettingsPage::buildOverlay() {
    m_overlay = LFOverlay::create();
    m_overlay->setVisible(false);
    m_overlay->setModal(true);
    m_overlay->setBarrierColor(0x66000000);
    m_overlay->setDismissOnBarrierTap(true);
}

void SettingsPage::buildUI() {
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

    std::weak_ptr<SettingsPage> weakSelf = std::static_pointer_cast<SettingsPage>(shared_from_this());
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

    auto title = createText(EnglishWordsI18n::tr("english_words.settings.title"), 22.0f, kTitleColor);
    title->matchParentWidth();
    title->setTextHAlign(LFTextHAlign::Center);
    title->setMaxLines(1);
    titleWrap->addChild(title);

    auto spacer = LFBox::create();
    spacer->setWidth(46.0f);
    spacer->setHeight(46.0f);
    headerRow->addChild(spacer);

    m_languageDropdown = LFDropdown::create(languageLabels());
    m_languageDropdown->matchParentWidth();
    m_languageDropdown->setDisplayMode(LFDropdownDisplayMode::Popup);
    m_languageDropdown->setOptionHeight(42.0f);
    m_languageDropdown->setMaxPanelHeight(160.0f);
    m_languageDropdown->setFontSize(15.0f);
    m_languageDropdown->setCornerRadius(18.0f);
    m_languageDropdown->setBorderColor(0xFFD8E4F1);
    m_languageDropdown->setTriggerBackgroundColor(0xFFFFFFFF, 0xFFEAF3FF);
    m_languageDropdown->setPanelBackgroundColor(0xFFFFFFFF);
    m_languageDropdown->setOptionBackgroundColor(0xFFFFFFFF, 0xFFF4F8FC, 0xFFEAF3FF);
    m_languageDropdown->setSelectedTextColor(0xFF3567A3);
    m_languageDropdown->getTriggerButton()->setHeight(52.0f);
    m_languageDropdown->getTriggerButton()->setShadow(0.0f, 8.0f, 20.0f, 0.0f, 0x10233B53);
    m_languageDropdown->setSelectedIndex(currentLanguageIndex(), false);
    m_languageDropdown->setOnSelectionChanged([weakSelf](int index, const std::string&) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }

        self->applySelectedLanguage(index);
        self->showRestartOverlay();
    });
    root->addChild(m_languageDropdown);
}

void SettingsPage::showRestartOverlay() {
    if (!m_overlay) {
        return;
    }

    auto card = LFLinear::createVertical();
    card->setWidth(280.0f);
    card->wrapContentHeight();
    card->setPadding(YGEdgeAll, 20.0f);
    card->setSpacing(16.0f);
    card->setBackgroundColor(0xFFFFFFFF);
    card->setBorderRadius(24.0f);
    card->setBorder(1.0f, kCardBorderColor);
    card->setShadow(0.0f, 16.0f, 32.0f, 0.0f, kCardShadowColor);

    auto message = createText(EnglishWordsI18n::tr("english_words.settings.restart_required"), 15.0f, kTitleColor);
    message->matchParentWidth();
    message->setTextHAlign(LFTextHAlign::Center);
    card->addChild(message);

    auto confirmButton = LFButton::create(EnglishWordsI18n::tr("english_words.settings.confirm"));
    confirmButton->matchParentWidth();
    confirmButton->setHeight(48.0f);
    confirmButton->setBorderRadius(18.0f);
    confirmButton->setBorder(1.0f, 0xFF3567A3);
    confirmButton->setFontSize(15.0f);
    confirmButton->setTextColor(0xFFFFFFFF);
    confirmButton->setBackgroundColor(LFButtonState::Normal, 0xFF3567A3);
    confirmButton->setBackgroundColor(LFButtonState::Pressed, 0xFF2C598F);
    std::weak_ptr<LFOverlay> weakOverlay = m_overlay;
    confirmButton->setOnClick([weakOverlay](LFButton*) {
        if (auto overlay = weakOverlay.lock()) {
            overlay->dismiss();
        }
    });
    card->addChild(confirmButton);

    m_overlay->show(card, LFBoxAlign::Center);
}

void SettingsPage::applySelectedLanguage(int index) {
    if (index < 0 || index >= static_cast<int>(kLanguageOptions.size())) {
        return;
    }

    LFI18n::setLanguage(kLanguageOptions[static_cast<size_t>(index)].tag);
}

int SettingsPage::currentLanguageIndex() const {
    const LFLocale current = LFI18n::getCurrentLanguage();
    const std::string languageCode = current.languageCode;

    if (languageCode == "en") {
        return 1;
    }
    if (languageCode == "ru") {
        return 2;
    }
    return 0;
}
