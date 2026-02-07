//
// Created by Chen Tong on 2026/2/2.
//

#ifndef BOOKSHELFPAGE_H
#define BOOKSHELFPAGE_H

#include "LFEngine.h"

class BookshelfPage : public LFPage {
public:
    // 传入 weak_ptr 避免循环引用
    static std::shared_ptr<BookshelfPage> create(std::weak_ptr<LFNavigator> nav);

private:
    void initUI();
    std::weak_ptr<LFNavigator> m_navigator;
};

#endif // BOOKSHELFPAGE_H
