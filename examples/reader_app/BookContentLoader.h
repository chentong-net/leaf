//
// Created by Chen Tong on 2026/2/22.
//

#ifndef BOOK_CONTENT_LOADER_H
#define BOOK_CONTENT_LOADER_H

#include <functional>
#include <string>

struct BookContentLoadResult {
    bool ok = false;
    std::string content;
    std::string error;
};

using BookContentLoadCallback = std::function<void(const BookContentLoadResult&)>;

class BookContentLoader {
public:
    static BookContentLoadResult loadTextFile(const std::string& filePath);
    static void loadTextFileAsync(const std::string& filePath, BookContentLoadCallback callback);
};

#endif // BOOK_CONTENT_LOADER_H
