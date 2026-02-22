//
// Created by Chen Tong on 2026/2/22.
//

#include "BookImportService.h"

#include "LFEngine.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <fstream>
#include <memory>
#include <sstream>
#include <sys/stat.h>
#include <vector>

#if defined(_WIN32)
#include <direct.h>
#endif

namespace {

constexpr size_t kCopyChunkSize = 256 * 1024;

BookImportResult makeError(const std::string& error, bool canceled = false) {
    BookImportResult result;
    result.ok = false;
    result.canceled = canceled;
    result.error = error;
    return result;
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

std::string fileNameFromPath(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) return path;
    return path.substr(slash + 1);
}

std::string trimExtension(const std::string& fileName) {
    const size_t dot = fileName.find_last_of('.');
    if (dot == std::string::npos || dot == 0) {
        return fileName;
    }
    return fileName.substr(0, dot);
}

std::string sanitizeFileName(const std::string& fileName) {
    if (fileName.empty()) return "book.txt";

    std::string out;
    out.reserve(fileName.size());
    for (char c : fileName) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            out.push_back('_');
        } else {
            out.push_back(c);
        }
    }
    if (out.empty()) return "book.txt";
    return out;
}

std::string makeUniquePrefix() {
    static std::atomic<uint64_t> counter{1};
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    return std::to_string(ms) + "_" + std::to_string(counter.fetch_add(1));
}

double nowSeconds() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count() / 1000.0;
}

bool copyFromPath(const std::string& sourcePath, std::ofstream& outFile, int64_t& bytesCopied) {
    bytesCopied = 0;
    std::ifstream inFile(sourcePath, std::ios::binary);
    if (!inFile.is_open()) {
        return false;
    }

    std::vector<char> buffer(kCopyChunkSize);
    while (inFile.good()) {
        inFile.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize readCount = inFile.gcount();
        if (readCount <= 0) {
            break;
        }

        outFile.write(buffer.data(), readCount);
        if (!outFile.good()) {
            return false;
        }
        bytesCopied += static_cast<int64_t>(readCount);
    }

    return !inFile.bad();
}

struct FdCopyContext {
    LFFileInfo source;
    std::string destPath;
    std::string safeTitle;
    std::string mimeType;
    size_t offset = 0;
    int64_t copiedBytes = 0;
    std::shared_ptr<std::ofstream> outFile;
    BookImportCallback callback;
};

void finishFdCopyWithError(const std::shared_ptr<FdCopyContext>& context, const std::string& error) {
    if (context->outFile && context->outFile->is_open()) {
        context->outFile->close();
    }
    if (context->callback) {
        context->callback(makeError(error));
    }
}

void finishFdCopySuccess(const std::shared_ptr<FdCopyContext>& context) {
    if (context->outFile && context->outFile->is_open()) {
        context->outFile->close();
    }

    BookImportResult result;
    result.ok = true;
    result.book.id = "book_" + makeUniquePrefix();
    result.book.title = context->safeTitle;
    result.book.filePath = context->destPath;
    result.book.mimeType = context->mimeType;
    result.book.size = context->copiedBytes;
    result.book.createdAt = nowSeconds();
    result.book.lastReadAt = result.book.createdAt;

    if (context->callback) {
        context->callback(result);
    }
}

void copyNextChunk(const std::shared_ptr<FdCopyContext>& context) {
    LFFileReadOptions readOptions;
    readOptions.offset = context->offset;
    readOptions.length = kCopyChunkSize;

    LFFilePicker::readFile(context->source, readOptions, [context](const LFFileReadResult& readResult) {
        if (!readResult.ok) {
            finishFdCopyWithError(context, readResult.error.empty() ? "read_source_failed" : readResult.error);
            return;
        }

        if (!context->outFile || !context->outFile->is_open()) {
            finishFdCopyWithError(context, "open_target_failed");
            return;
        }

        if (!readResult.bytes.empty()) {
            context->outFile->write(
                reinterpret_cast<const char*>(readResult.bytes.data()),
                static_cast<std::streamsize>(readResult.bytes.size())
            );
            if (!context->outFile->good()) {
                finishFdCopyWithError(context, "write_target_failed");
                return;
            }
        }

        context->offset += readResult.bytes.size();
        context->copiedBytes += static_cast<int64_t>(readResult.bytes.size());

        if (readResult.eof || readResult.bytes.empty()) {
            finishFdCopySuccess(context);
            return;
        }

        LFEngine::getInstance().addFrameTask([context]() mutable {
            copyNextChunk(context);
            return false;
        });
    });
}

} // namespace

void BookImportService::pickAndImport(const std::string& booksDirectory, BookImportCallback callback) {
    if (!callback) return;

    LFFilePickerOptions options;
    options.mediaType = LFFilePickerMediaType::Any;
    options.copyToSandbox = true;

    LFFilePicker::pickFile(options, [booksDirectory, callback = std::move(callback)](const LFFilePickResult& pickResult) mutable {
        if (!pickResult.ok) {
            callback(makeError(pickResult.error.empty() ? "pick_failed" : pickResult.error, pickResult.canceled));
            return;
        }
        if (pickResult.canceled) {
            callback(makeError("pick_canceled", true));
            return;
        }
        importPickedFile(pickResult.file, booksDirectory, std::move(callback));
    });
}

void BookImportService::importPickedFile(const LFFileInfo& sourceFile,
                                         const std::string& booksDirectory,
                                         BookImportCallback callback) {
    if (!callback) return;

    if (booksDirectory.empty()) {
        callback(makeError("books_directory_empty"));
        return;
    }

    if (!ensureDirectory(booksDirectory)) {
        callback(makeError("create_books_directory_failed"));
        return;
    }

    const std::string sourceName = !sourceFile.name.empty() ? sourceFile.name : fileNameFromPath(sourceFile.path);
    const std::string safeName = sanitizeFileName(sourceName);
    const std::string title = trimExtension(safeName);
    const std::string destPath = joinPath(booksDirectory, makeUniquePrefix() + "_" + safeName);

    auto outFile = std::make_shared<std::ofstream>(destPath, std::ios::binary | std::ios::trunc);
    if (!outFile->is_open()) {
        callback(makeError("open_target_failed"));
        return;
    }

    if (!sourceFile.path.empty()) {
        int64_t copiedBytes = 0;
        if (!copyFromPath(sourceFile.path, *outFile, copiedBytes)) {
            outFile->close();
            callback(makeError("copy_from_path_failed"));
            return;
        }

        outFile->close();

        BookImportResult result;
        result.ok = true;
        result.book.id = "book_" + makeUniquePrefix();
        result.book.title = title;
        result.book.filePath = destPath;
        result.book.mimeType = sourceFile.mimeType;
        result.book.size = copiedBytes > 0 ? copiedBytes : sourceFile.size;
        result.book.createdAt = nowSeconds();
        result.book.lastReadAt = result.book.createdAt;
        callback(result);
        return;
    }

    if (sourceFile.fileId.empty()) {
        outFile->close();
        callback(makeError("read_source_unavailable"));
        return;
    }

    auto context = std::make_shared<FdCopyContext>();
    context->source = sourceFile;
    context->destPath = destPath;
    context->safeTitle = title;
    context->mimeType = sourceFile.mimeType;
    context->outFile = outFile;
    context->callback = std::move(callback);
    copyNextChunk(context);
}
