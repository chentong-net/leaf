//
// Created by Chen Tong on 2026/2/22.
//

#ifndef BOOK_CONTENT_LOADER_H
#define BOOK_CONTENT_LOADER_H

#include <string>

struct BookContentLoadResult {
    bool ok = false;
    std::string content;
    std::string error;
};

class BookContentLoader {
public:
    static BookContentLoadResult loadTextFile(const std::string& filePath);
};

#endif // BOOK_CONTENT_LOADER_H
