//
// Created by Chen Tong on 2026/2/22.
//

#ifndef BOOK_IMPORT_SERVICE_H
#define BOOK_IMPORT_SERVICE_H

#include "BookModel.h"
#include "LFFilePicker.h"

#include <functional>
#include <string>

struct BookImportResult {
    bool ok = false;
    bool canceled = false;
    BookRecord book;
    std::string error;
};

using BookImportCallback = std::function<void(const BookImportResult&)>;

class BookImportService {
public:
    static void pickAndImport(const std::string& booksDirectory, BookImportCallback callback);

private:
    static void importPickedFile(const LFFileInfo& sourceFile,
                                 const std::string& booksDirectory,
                                 BookImportCallback callback);
};

#endif // BOOK_IMPORT_SERVICE_H
