#include "EnglishWordsApp.h"

#include "EnglishWordsDataManager.h"
#include "MainPage.h"

LFNode::Ptr EnglishWordsApp::start() {
    EnglishWordsDataManager::preloadLevels();

    auto navigator = LFNavigator::create();
    navigator->push(MainPage::create(), false);
    return navigator;
}
