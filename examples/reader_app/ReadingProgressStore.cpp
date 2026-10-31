//
// Created by Chen Tong on 2026/2/22.
//

#include "ReadingProgressStore.h"

#include "LFJSONParser.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

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
    const std::filesystem::path targetPath = std::filesystem::u8path(path);
    std::filesystem::path temporaryPath = targetPath;
    temporaryPath += ".tmp";

    std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    output.flush();
    const bool writeSucceeded = output.good();
    output.close();
    if (!writeSucceeded) {
        std::error_code removeError;
        std::filesystem::remove(temporaryPath, removeError);
        return false;
    }

#if defined(_WIN32)
    const bool replaced = MoveFileExW(
            temporaryPath.wstring().c_str(),
            targetPath.wstring().c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
    ) != 0;
#else
    const bool replaced = std::rename(
            temporaryPath.string().c_str(),
            targetPath.string().c_str()
    ) == 0;
#endif
    if (!replaced) {
        std::error_code removeError;
        std::filesystem::remove(temporaryPath, removeError);
    }
    return replaced;
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
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buffer[7];
                    std::snprintf(
                            buffer,
                            sizeof(buffer),
                            "\\u%04x",
                            static_cast<unsigned char>(c)
                    );
                    escaped += buffer;
                } else {
                    escaped += c;
                }
                break;
        }
    }
    return escaped;
}

double sanitizeTimestamp(double value) {
    return std::isfinite(value) && value >= 0.0 ? value : 0.0;
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
    if (!loadFromDiskLocked()) {
        m_initialized = false;
        return false;
    }
    return true;
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

bool FileReadingProgressStore::put(const std::string& bookId, const ReadingProgress& progress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (bookId.empty()) return false;

    ReadingProgress sanitized = progress;
    sanitized.pageIndex = std::max(0, sanitized.pageIndex);
    sanitized.updatedAt = sanitizeTimestamp(sanitized.updatedAt);

    auto existing = m_data.find(bookId);
    const bool hadPrevious = existing != m_data.end();
    const ReadingProgress previous = hadPrevious ? existing->second : ReadingProgress{};
    m_data[bookId] = sanitized;
    if (m_initialized && !persistToDiskLocked()) {
        if (hadPrevious) {
            m_data[bookId] = previous;
        } else {
            m_data.erase(bookId);
        }
        return false;
    }
    return true;
}

bool FileReadingProgressStore::clear(const std::string& bookId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_data.find(bookId);
    if (it == m_data.end()) {
        return true;
    }

    const ReadingProgress previous = it->second;
    m_data.erase(bookId);
    if (m_initialized && !persistToDiskLocked()) {
        m_data[bookId] = previous;
        return false;
    }
    return true;
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
                const double offset = item->at("pageStartOffset").asDouble();
                if (offset >= 0.0 &&
                    offset <= static_cast<double>(std::numeric_limits<size_t>::max())) {
                    progress.pageStartOffset = static_cast<size_t>(offset);
                }
            }
            if (item->contains("pageIndex")) {
                progress.pageIndex = std::max(0, item->at("pageIndex").asInt());
            }
            if (item->contains("updatedAt")) {
                progress.updatedAt = sanitizeTimestamp(item->at("updatedAt").asDouble());
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
