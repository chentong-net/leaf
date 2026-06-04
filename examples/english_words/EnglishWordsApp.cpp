#include "EnglishWordsApp.h"

#include "MainPage.h"

LFNode::Ptr EnglishWordsApp::start() {
    auto navigator = LFNavigator::create();
    navigator->push(MainPage::create(), false);
    return navigator;
}
