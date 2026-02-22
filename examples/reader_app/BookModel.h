//
// Created by Chen Tong on 2026/2/22.
//

#ifndef BOOK_MODEL_H
#define BOOK_MODEL_H

#include <cstdint>
#include <string>

struct BookRecord {
    std::string id;
    std::string title;
    std::string filePath;
    std::string mimeType;
    int64_t size = 0;
    double createdAt = 0.0;
    double lastReadAt = 0.0;
};

#endif // BOOK_MODEL_H
