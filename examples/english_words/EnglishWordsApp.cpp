#include "EnglishWordsApp.h"

#include "EnglishWordsDataManager.h"
#include "LFI18n.h"
#include "MainPage.h"

LFNode::Ptr EnglishWordsApp::start() {
    LFI18n::initializeFromAsset("i18n.json");
    EnglishWordsDataManager::preloadLevels();

    auto navigator = LFNavigator::create();
    navigator->push(MainPage::create(), false);
    return navigator;
}
