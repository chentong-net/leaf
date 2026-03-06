//
// Created by Chen Tong on 2026/2/22.
//

#include "BookRepository.h"

#include "LFJSONParser.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

bool isValidUtf8(const std::string& text) {
    size_t i = 0;
    const size_t n = text.size();
    while (i < n) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        size_t need = 0;
        if ((c & 0x80) == 0x00) {
            ++i;
            continue;
        } else if ((c & 0xE0) == 0xC0) {
            need = 1;
            if (c < 0xC2) return false;
        } else if ((c & 0xF0) == 0xE0) {
            need = 2;
        } else if ((c & 0xF8) == 0xF0) {
            need = 3;
            if (c > 0xF4) return false;
        } else {
            return false;
        }

        if (i + need >= n) return false;
        for (size_t j = 1; j <= need; ++j) {
            const unsigned char cc = static_cast<unsigned char>(text[i + j]);
            if ((cc & 0xC0) != 0x80) return false;
        }

        if (need == 2) {
            const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
            if (c == 0xE0 && c1 < 0xA0) return false;
            if (c == 0xED && c1 >= 0xA0) return false;
        } else if (need == 3) {
            const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
            if (c == 0xF0 && c1 < 0x90) return false;
            if (c == 0xF4 && c1 >= 0x90) return false;
        }

        i += need + 1;
    }
    return true;
}

#if defined(_WIN32)
std::string convertAcpToUtf8(const std::string& text) {
    if (text.empty()) return "";

    const int wideCount = MultiByteToWideChar(
        CP_ACP,
        MB_ERR_INVALID_CHARS,
        text.c_str(),
        static_cast<int>(text.size()),
        nullptr,
        0
    );
    if (wideCount <= 0) {
        return text;
    }

    std::wstring wide(static_cast<size_t>(wideCount), L'\0');
    const int wideConverted = MultiByteToWideChar(
        CP_ACP,
        MB_ERR_INVALID_CHARS,
        text.c_str(),
        static_cast<int>(text.size()),
        wide.data(),
        wideCount
    );
    if (wideConverted <= 0) {
        return text;
    }

    const int utf8Count = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );
    if (utf8Count <= 0) {
        return text;
    }

    std::string utf8(static_cast<size_t>(utf8Count), '\0');
    const int utf8Converted = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        utf8.data(),
        utf8Count,
        nullptr,
        nullptr
    );
    if (utf8Converted <= 0) {
        return text;
    }
    return utf8;
}
#endif

std::string normalizeUtf8(const std::string& text) {
    if (text.empty()) return text;
    if (isValidUtf8(text)) return text;
#if defined(_WIN32)
    return convertAcpToUtf8(text);
#else
    return text;
#endif
}

bool isAbsolutePath(const std::string& path) {
    if (path.empty()) return false;
    if (path[0] == '/' || path[0] == '\\') return true;
    return path.size() > 1 && std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':';
}

std::string joinPath(const std::string& base, const std::string& child) {
    if (base.empty()) return child;
    if (child.empty()) return base;
    if (isAbsolutePath(child)) return child;

    const char last = base.back();
    if (last == '/' || last == '\\') {
        return base + child;
    }
    return base + "/" + child;
}

bool ensureDirectory(const std::string& path) {
    if (path.empty()) return false;

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::u8path(path), ec);
    return !ec;
}

bool ensureParentDirectory(const std::string& filePath) {
    std::error_code ec;
    const auto parent = std::filesystem::u8path(filePath).parent_path();
    if (parent.empty()) return true;
    std::filesystem::create_directories(parent, ec);
    return !ec;
}

bool readTextFile(const std::string& path, std::string& out) {
    std::ifstream input(std::filesystem::u8path(path), std::ios::binary);
    if (!input.is_open()) {
        out.clear();
        return false;
    }

    std::ostringstream oss;
    oss << input.rdbuf();
    out = oss.str();
    return true;
}

bool writeTextFile(const std::string& path, const std::string& content) {
    if (!ensureParentDirectory(path)) {
        return false;
    }

    std::ofstream output(std::filesystem::u8path(path), std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    return output.good();
}

std::string trimExtension(const std::string& fileName) {
    const size_t dot = fileName.find_last_of('.');
    if (dot == std::string::npos || dot == 0) {
        return fileName;
    }
    return fileName.substr(0, dot);
}

} // namespace

BookRepository& BookRepository::getInstance() {
    static BookRepository instance;
    return instance;
}

bool BookRepository::initialize(const std::string& appSupportPath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (appSupportPath.empty()) {
        return false;
    }

    m_rootDirectory = joinPath(appSupportPath, "reader_app");
    m_booksDirectory = joinPath(m_rootDirectory, "books");
    m_storeFilePath = joinPath(m_rootDirectory, "bookshelf.json");

    if (!ensureDirectory(m_rootDirectory) || !ensureDirectory(m_booksDirectory)) {
        return false;
    }

    m_initialized = true;
    return loadFromDiskLocked();
}

bool BookRepository::isInitialized() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_initialized;
}

std::string BookRepository::getRootDirectory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_rootDirectory;
}

std::string BookRepository::getBooksDirectory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_booksDirectory;
}

std::vector<BookRecord> BookRepository::listBooks() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto books = m_books;
    std::sort(books.begin(), books.end(), [](const BookRecord& a, const BookRecord& b) {
        if (a.lastReadAt != b.lastReadAt) {
            return a.lastReadAt > b.lastReadAt;
        }
        return a.createdAt > b.createdAt;
    });
    return books;
}

bool BookRepository::upsertBook(const BookRecord& incoming) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || incoming.filePath.empty()) {
        return false;
    }

    BookRecord book = incoming;
    book.id = normalizeUtf8(book.id);
    book.title = normalizeUtf8(book.title);
    book.filePath = normalizeUtf8(book.filePath);
    book.mimeType = normalizeUtf8(book.mimeType);
    if (book.title.empty()) {
        const size_t slash = book.filePath.find_last_of("/\\");
        const std::string fileName = slash == std::string::npos ? book.filePath : book.filePath.substr(slash + 1);
        book.title = trimExtension(fileName);
    }

    auto sameBook = [&book](const BookRecord& item) {
        if (!book.id.empty() && item.id == book.id) return true;
        if (!book.filePath.empty() && item.filePath == book.filePath) return true;
        if (!book.title.empty() && item.title == book.title && book.size > 0 && item.size == book.size) return true;
        return false;
    };

    auto it = std::find_if(m_books.begin(), m_books.end(), sameBook);
    if (it != m_books.end()) {
        const std::string existedId = it->id;
        const double existedCreatedAt = it->createdAt;
        if (!existedId.empty()) {
            book.id = existedId;
        }
        if (book.createdAt <= 0) {
            book.createdAt = existedCreatedAt;
        }
        *it = book;
    } else {
        m_books.push_back(book);
    }

    return persistToDiskLocked();
}

bool BookRepository::removeBook(const std::string& bookId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || bookId.empty()) {
        return false;
    }

    const auto oldSize = m_books.size();
    m_books.erase(
        std::remove_if(m_books.begin(), m_books.end(), [&bookId](const BookRecord& book) {
            return book.id == bookId;
        }),
        m_books.end()
    );

    if (m_books.size() == oldSize) {
        return false;
    }

    return persistToDiskLocked();
}

bool BookRepository::findBook(const std::string& bookId, BookRecord& outBook) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || bookId.empty()) {
        return false;
    }

    auto it = std::find_if(m_books.begin(), m_books.end(), [&bookId](const BookRecord& item) {
        return item.id == bookId;
    });
    if (it == m_books.end()) {
        return false;
    }

    outBook = *it;
    return true;
}

bool BookRepository::touchBook(const std::string& bookId, double timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || bookId.empty()) {
        return false;
    }

    auto it = std::find_if(m_books.begin(), m_books.end(), [&bookId](const BookRecord& item) {
        return item.id == bookId;
    });
    if (it == m_books.end()) {
        return false;
    }

    it->lastReadAt = timestamp;
    return persistToDiskLocked();
}

bool BookRepository::loadFromDiskLocked() {
    m_books.clear();

    std::string raw;
    if (!readTextFile(m_storeFilePath, raw)) {
        return true;
    }

    if (raw.empty()) {
        return true;
    }

    bool migratedEncoding = false;
    if (!isValidUtf8(raw)) {
        const std::string converted = normalizeUtf8(raw);
        if (converted != raw) {
            raw = converted;
            migratedEncoding = true;
        }
    }

    try {
        auto root = LFJSONParser::parse(raw);
        if (!root || !root->contains("books")) {
            return true;
        }

        auto& booksValue = root->at("books");
        if (!booksValue.isArray()) {
            return true;
        }

        for (auto& itemValue : booksValue.asArray()) {
            if (!itemValue.isObject()) {
                continue;
            }
            auto item = itemValue.asObject();
            BookRecord book;
            if (item->contains("id")) book.id = normalizeUtf8(item->at("id").asString());
            if (item->contains("title")) book.title = normalizeUtf8(item->at("title").asString());
            if (item->contains("filePath")) book.filePath = normalizeUtf8(item->at("filePath").asString());
            if (item->contains("mimeType")) book.mimeType = normalizeUtf8(item->at("mimeType").asString());
            if (item->contains("size")) book.size = static_cast<int64_t>(item->at("size").asDouble());
            if (item->contains("createdAt")) book.createdAt = item->at("createdAt").asDouble();
            if (item->contains("lastReadAt")) book.lastReadAt = item->at("lastReadAt").asDouble();

            if (!book.id.empty() && !book.filePath.empty()) {
                m_books.push_back(std::move(book));
            }
        }

        if (migratedEncoding) {
            return persistToDiskLocked();
        }
        return true;
    } catch (...) {
        m_books.clear();
        return false;
    }
}

bool BookRepository::persistToDiskLocked() const {
    std::ostringstream oss;
    oss << "{\"version\":1,\"books\":[";

    bool first = true;
    for (const auto& book : m_books) {
        if (book.id.empty() || book.filePath.empty()) {
            continue;
        }
        if (!first) {
            oss << ',';
        }
        first = false;

        oss << '{'
            << "\"id\":\"" << escapeJson(book.id) << "\","
            << "\"title\":\"" << escapeJson(book.title) << "\","
            << "\"filePath\":\"" << escapeJson(book.filePath) << "\","
            << "\"mimeType\":\"" << escapeJson(book.mimeType) << "\","
            << "\"size\":" << static_cast<long long>(book.size) << ','
            << "\"createdAt\":" << book.createdAt << ','
            << "\"lastReadAt\":" << book.lastReadAt
            << '}';
    }

    oss << "]}";
    return writeTextFile(m_storeFilePath, oss.str());
}

std::string BookRepository::escapeJson(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size() + 8);
    for (char c : text) {
        switch (c) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += c;
                break;
        }
    }
    return escaped;
}
