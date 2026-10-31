//
// Created by Chen Tong on 2026/2/22.
//

#include "BookContentLoader.h"

#include "LFEngine.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>

BookContentLoadResult BookContentLoader::loadTextFile(const std::string& filePath) {
    BookContentLoadResult result;
    if (filePath.empty()) {
        result.error = "empty_path";
        return result;
    }

    std::ifstream file(std::filesystem::u8path(filePath), std::ios::binary);
    if (!file.is_open()) {
        result.error = "open_file_failed";
        return result;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    if (!file.good() && !file.eof()) {
        result.error = "read_file_failed";
        return result;
    }

    result.ok = true;
    result.content = oss.str();
    return result;
}

void BookContentLoader::loadTextFileAsync(
        const std::string& filePath,
        BookContentLoadCallback callback) {
    if (!callback) {
        return;
    }
    if (filePath.empty()) {
        callback({false, "", "empty_path"});
        return;
    }

    struct LoadContext {
        std::string filePath;
        std::shared_ptr<std::ifstream> file;
        std::string content;
        std::vector<char> buffer = std::vector<char>(256 * 1024);
        BookContentLoadCallback callback;
    };
    auto context = std::make_shared<LoadContext>();
    context->filePath = filePath;
    context->callback = std::move(callback);

    LFEngine::getInstance().addFrameTask([context]() mutable {
        if (!context->file) {
            context->file = std::make_shared<std::ifstream>(
                    std::filesystem::u8path(context->filePath),
                    std::ios::binary
            );
            if (!context->file->is_open()) {
                context->callback({false, "", "open_file_failed"});
                return false;
            }
        }

        context->file->read(
                context->buffer.data(),
                static_cast<std::streamsize>(context->buffer.size())
        );
        const std::streamsize readCount = context->file->gcount();
        if (readCount > 0) {
            context->content.append(
                    context->buffer.data(),
                    static_cast<size_t>(readCount)
            );
        }

        if (context->file->bad()) {
            context->callback({false, "", "read_file_failed"});
            return false;
        }
        if (context->file->eof()) {
            BookContentLoadResult result;
            result.ok = true;
            result.content = std::move(context->content);
            context->callback(result);
            return false;
        }
        return true;
    });
}
