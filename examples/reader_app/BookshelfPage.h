//
// Created by Chen Tong on 2026/2/2.
//

#ifndef BOOKSHELFPAGE_H
#define BOOKSHELFPAGE_H

#include "BookModel.h"
#include "LFEngine.h"

#include <string>
#include <vector>

class BookshelfPage : public LFPage {
public:
    static std::shared_ptr<BookshelfPage> create(std::weak_ptr<LFNavigator> nav);

private:
    void initUI();
    void setupStorage();
    void refreshGrid();
    void addImportEntry();
    void addBookEntry(const BookRecord& book, int index);
    void startImportBook();
    void openBook(const BookRecord& book);
    void setStatusText(const std::string& text, uint32_t color);

    std::shared_ptr<LFText> m_statusText;
    std::shared_ptr<LFGrid> m_grid;
    std::vector<LFNode::Ptr> m_gridItems;
    std::vector<BookRecord> m_books;
    std::string m_booksDirectory;
    bool m_importing = false;

    std::weak_ptr<LFNavigator> m_navigator;
};

#endif // BOOKSHELFPAGE_H
