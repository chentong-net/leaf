#include "EnglishWordsApp.h"

#include "MainPage.h"

std::shared_ptr<EnglishWordsApp> EnglishWordsApp::create() {
    return std::make_shared<EnglishWordsApp>();
}

LFNode::Ptr EnglishWordsApp::start() {
    m_navigator = LFNavigator::create();
    m_navigator->push(MainPage::create(m_navigator), false);
    return m_navigator;
}
