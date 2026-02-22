//
// Created by Chen Tong on 2026/2/12.
//

#ifndef READING_PROGRESS_STORE_H
#define READING_PROGRESS_STORE_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

/**
 * 阅读进度记录
 * 当前版本仅保存在内存中，后续可平滑迁移为本地持久化
 */
struct ReadingProgress {
    size_t pageStartOffset = 0;
    int pageIndex = 0;
    double updatedAt = 0.0;
};

/**
 * 阅读进度仓库抽象
 * ReaderPage 只依赖该接口，方便后续替换为文件/数据库存储
 */
class IReadingProgressStore {
public:
    virtual ~IReadingProgressStore() = default;
    virtual bool get(const std::string& bookId, ReadingProgress& out) = 0;
    virtual void put(const std::string& bookId, const ReadingProgress& progress) = 0;
    virtual void clear(const std::string& bookId) = 0;
};

/**
 * 内存版阅读进度仓库
 */
class InMemoryReadingProgressStore : public IReadingProgressStore {
public:
    static InMemoryReadingProgressStore& getInstance() {
        static InMemoryReadingProgressStore instance;
        return instance;
    }

    bool get(const std::string& bookId, ReadingProgress& out) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_data.find(bookId);
        if (it == m_data.end()) return false;
        out = it->second;
        return true;
    }

    void put(const std::string& bookId, const ReadingProgress& progress) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data[bookId] = progress;
    }

    void clear(const std::string& bookId) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.erase(bookId);
    }

private:
    InMemoryReadingProgressStore() = default;
    std::unordered_map<std::string, ReadingProgress> m_data;
    std::mutex m_mutex;
};

class FileReadingProgressStore : public IReadingProgressStore {
public:
    static FileReadingProgressStore& getInstance();

    bool initialize(const std::string& filePath);
    bool isInitialized() const;

    bool get(const std::string& bookId, ReadingProgress& out) override;
    void put(const std::string& bookId, const ReadingProgress& progress) override;
    void clear(const std::string& bookId) override;

private:
    FileReadingProgressStore() = default;

    bool loadFromDiskLocked();
    bool persistToDiskLocked() const;

    mutable std::mutex m_mutex;
    bool m_initialized = false;
    std::string m_filePath;
    std::map<std::string, ReadingProgress> m_data;
};

#endif // READING_PROGRESS_STORE_H
