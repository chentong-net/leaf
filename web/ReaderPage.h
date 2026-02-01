//
// Created by Leaf Engine Team.
// ReaderPage.h
//

#ifndef READERPAGE_H
#define READERPAGE_H

#include "LFPage.h"
#include "LFLinear.h"
#include "LFBox.h"
#include "LFPageView.h" // 核心组件
#include <string>

class ReaderPage : public LFPage {
public:
    using Ptr = std::shared_ptr<ReaderPage>;

    /**
     * 创建阅读页
     * @param bookTitle 书名
     * @param content 书籍内容
     */
    static Ptr create(const std::string& bookTitle, const std::string& content);

    ReaderPage();
    virtual ~ReaderPage() = default;

private:
    void initLayout(const std::string& title, const std::string& content);
    void setupTopBar(const std::string& title);

    // 核心逻辑：切分文本并初始化 Pager
    void splitContentAndInit(const std::string& content);

    // 交互逻辑：切换菜单显示/隐藏
    void toggleMenu();

    // UI 组件
    std::shared_ptr<LFLinear> m_topBar;
    LFPageView::Ptr m_pageView;

    // 状态
    bool m_isMenuVisible = true;
};

#endif // READERPAGE_H