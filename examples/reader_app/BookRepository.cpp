//
// Created by Chen Tong on 2026/2/22.
//

#include "BookRepository.h"

#include "LFJSONParser.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#if defined(_WIN32)
#include <direct.h>
#endif

namespace {

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

    std::string normalized = path;
    for (char& c : normalized) {
        if (c == '\\') c = '/';
    }

    std::string current;
    size_t index = 0;

    if (normalized.size() > 1 && std::isalpha(static_cast<unsigned char>(normalized[0])) && normalized[1] == ':') {
        current = normalized.substr(0, 2);
        index = 2;
    } else if (!normalized.empty() && normalized[0] == '/') {
        current = "/";
        index = 1;
    }

    while (index < normalized.size()) {
        while (index < normalized.size() && normalized[index] == '/') {
            ++index;
        }
        if (index >= normalized.size()) break;

        const size_t nextSlash = normalized.find('/', index);
        const std::string part = normalized.substr(index, nextSlash == std::string::npos ? std::string::npos : (nextSlash - index));
        if (part.empty()) {
            index = nextSlash == std::string::npos ? normalized.size() : nextSlash + 1;
            continue;
        }

        if (!current.empty() && current.back() != '/') current += '/';
        current += part;

#if defined(_WIN32)
        const int rc = _mkdir(current.c_str());
#else
        const int rc = mkdir(current.c_str(), 0755);
#endif
        if (rc != 0 && errno != EEXIST) {
            return false;
        }

        if (nextSlash == std::string::npos) break;
        index = nextSlash + 1;
    }

    return true;
}

bool ensureParentDirectory(const std::string& filePath) {
    const size_t pos = filePath.find_last_of("/\\");
    if (pos == std::string::npos) return true;
    return ensureDirectory(filePath.substr(0, pos));
}

bool readTextFile(const std::string& path, std::string& out) {
    std::ifstream input(path, std::ios::binary);
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

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
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
            if (item->contains("id")) book.id = item->at("id").asString();
            if (item->contains("title")) book.title = item->at("title").asString();
            if (item->contains("filePath")) book.filePath = item->at("filePath").asString();
            if (item->contains("mimeType")) book.mimeType = item->at("mimeType").asString();
            if (item->contains("size")) book.size = static_cast<int64_t>(item->at("size").asDouble());
            if (item->contains("createdAt")) book.createdAt = item->at("createdAt").asDouble();
            if (item->contains("lastReadAt")) book.lastReadAt = item->at("lastReadAt").asDouble();

            if (!book.id.empty() && !book.filePath.empty()) {
                m_books.push_back(std::move(book));
            }
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
