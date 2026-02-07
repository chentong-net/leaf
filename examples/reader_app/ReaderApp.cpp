//
// Created by Chen Tong on 2026/2/2.
//

#include "ReaderApp.h"
#include "BookshelfPage.h"
#include "ProfilePage.h"

std::shared_ptr<ReaderApp> ReaderApp::create() {
    return std::make_shared<ReaderApp>();
}

LFNode::Ptr ReaderApp::start() {
    // 1. 创建导航器
    m_navigator = LFNavigator::create();

    // 2. 创建 Tab 组件
    auto tab = LFTab::create();

    // 3. 添加子页面
    // 显式传递 navigator 给书架页，实现解耦
    tab->addTab("书架", BookshelfPage::create(m_navigator), "icon-read-unselect.png", "icon-read-selected.png");
    tab->addTab("开发者信息", ProfilePage::create(), "icon-my-unselect.png", "icon-my-selected.png");

    // 4. 创建宿主 Page 来容纳 Tab
    auto rootPage = LFPage::create();
    rootPage->addChild(tab);

    // 5. 将宿主页推入栈底
    m_navigator->push(rootPage, false);

    return m_navigator;
}
