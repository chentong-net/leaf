//
// Created by Leaf Engine Team.
// ReaderPage.h
//

#ifndef READERPAGE_H
#define READERPAGE_H

#include "LFEngine.h"
#include "TextSplitter.h"

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

    SplitIterator m_splitIter;
    std::string m_fullContent; // 解决空指针问题
};

#endif // READERPAGE_H