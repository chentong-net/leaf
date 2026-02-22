//
// Created by Chen Tong on 2026/2/22.
//

#include "ReadingProgressStore.h"

#include "LFJSONParser.h"

#include <cerrno>
#include <cctype>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#if defined(_WIN32)
#include <direct.h>
#endif

namespace {

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

std::string escapeJson(const std::string& text) {
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

} // namespace

FileReadingProgressStore& FileReadingProgressStore::getInstance() {
    static FileReadingProgressStore instance;
    return instance;
}

bool FileReadingProgressStore::initialize(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (filePath.empty()) {
        return false;
    }
    m_filePath = filePath;
    m_initialized = true;
    return loadFromDiskLocked();
}

bool FileReadingProgressStore::isInitialized() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_initialized;
}

bool FileReadingProgressStore::get(const std::string& bookId, ReadingProgress& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_data.find(bookId);
    if (it == m_data.end()) return false;
    out = it->second;
    return true;
}

void FileReadingProgressStore::put(const std::string& bookId, const ReadingProgress& progress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (bookId.empty()) return;
    m_data[bookId] = progress;
    if (m_initialized) {
        persistToDiskLocked();
    }
}

void FileReadingProgressStore::clear(const std::string& bookId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.erase(bookId);
    if (m_initialized) {
        persistToDiskLocked();
    }
}

bool FileReadingProgressStore::loadFromDiskLocked() {
    m_data.clear();
    std::string raw;
    if (!readTextFile(m_filePath, raw)) {
        return true;
    }
    if (raw.empty()) {
        return true;
    }

    try {
        auto root = LFJSONParser::parse(raw);
        if (!root || !root->contains("items")) {
            return true;
        }

        auto& itemsValue = root->at("items");
        if (!itemsValue.isArray()) {
            return true;
        }

        for (auto& value : itemsValue.asArray()) {
            if (!value.isObject()) continue;
            auto item = value.asObject();
            if (!item->contains("bookId")) continue;

            ReadingProgress progress;
            if (item->contains("pageStartOffset")) {
                progress.pageStartOffset = static_cast<size_t>(item->at("pageStartOffset").asDouble());
            }
            if (item->contains("pageIndex")) {
                progress.pageIndex = item->at("pageIndex").asInt();
            }
            if (item->contains("updatedAt")) {
                progress.updatedAt = item->at("updatedAt").asDouble();
            }

            m_data[item->at("bookId").asString()] = progress;
        }
        return true;
    } catch (...) {
        m_data.clear();
        return false;
    }
}

bool FileReadingProgressStore::persistToDiskLocked() const {
    if (m_filePath.empty()) return false;

    std::ostringstream oss;
    oss << "{\"version\":1,\"items\":[";
    bool first = true;
    for (const auto& item : m_data) {
        if (item.first.empty()) continue;
        if (!first) {
            oss << ',';
        }
        first = false;
        oss << '{'
            << "\"bookId\":\"" << escapeJson(item.first) << "\","
            << "\"pageStartOffset\":" << static_cast<unsigned long long>(item.second.pageStartOffset) << ','
            << "\"pageIndex\":" << item.second.pageIndex << ','
            << "\"updatedAt\":" << item.second.updatedAt
            << '}';
    }
    oss << "]}";
    return writeTextFile(m_filePath, oss.str());
}
