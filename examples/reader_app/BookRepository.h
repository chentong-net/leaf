//
// Created by Chen Tong on 2026/2/22.
//

#ifndef BOOK_REPOSITORY_H
#define BOOK_REPOSITORY_H

#include "BookModel.h"

#include <mutex>
#include <string>
#include <vector>

class BookRepository {
public:
    static BookRepository& getInstance();

    bool initialize(const std::string& appSupportPath);
    bool isInitialized() const;

    std::string getRootDirectory() const;
    std::string getBooksDirectory() const;

    std::vector<BookRecord> listBooks() const;
    bool upsertBook(const BookRecord& book);
    bool removeBook(const std::string& bookId);
    bool findBook(const std::string& bookId, BookRecord& outBook) const;
    bool touchBook(const std::string& bookId, double timestamp);

private:
    BookRepository() = default;

    bool loadFromDiskLocked();
    bool persistToDiskLocked() const;

    static std::string escapeJson(const std::string& text);

    mutable std::mutex m_mutex;
    bool m_initialized = false;
    std::string m_rootDirectory;
    std::string m_booksDirectory;
    std::string m_storeFilePath;
    std::vector<BookRecord> m_books;
};

#endif // BOOK_REPOSITORY_H
