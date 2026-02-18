//
// Created by Chen Tong on 2026/2/18.
//

#include "LFFilePicker.h"
#include "LFJSONParser.h"
#include "plugin/LFNativeSender.h"

#include <fstream>
#include <sstream>

#if defined(__ANDROID__) || defined(__APPLE__) || defined(__linux__) || defined(__OHOS__)
#include <unistd.h>
#endif

namespace {

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

LFFileReadResult makeReadError(const std::string& error) {
    LFFileReadResult result;
    result.ok = false;
    result.error = error;
    return result;
}

LFFilePickResult makePickError(const std::string& error, bool canceled = false) {
    LFFilePickResult result;
    result.ok = false;
    result.canceled = canceled;
    result.error = error;
    return result;
}

LFFileReadResult readChunkFromPath(const std::string& path, const LFFileReadOptions& options) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return makeReadError("open_path_failed");
    }

    file.seekg(0, std::ios::end);
    const std::streamoff totalSize = file.tellg();
    if (totalSize < 0) {
        return makeReadError("read_size_failed");
    }

    const size_t offset = options.offset;
    const size_t length = options.length == 0 ? (256 * 1024) : options.length;

    LFFileReadResult result;
    result.ok = true;
    if (static_cast<std::streamoff>(offset) >= totalSize) {
        result.eof = true;
        return result;
    }

    file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    result.bytes.resize(length);
    file.read(reinterpret_cast<char*>(result.bytes.data()), static_cast<std::streamsize>(length));
    const std::streamsize bytesRead = file.gcount();
    if (bytesRead < 0) {
        return makeReadError("read_path_failed");
    }
    result.bytes.resize(static_cast<size_t>(bytesRead));
    result.eof = static_cast<std::streamoff>(offset + result.bytes.size()) >= totalSize;
    return result;
}

std::string buildPickArgs(const LFFilePickerOptions& options) {
    std::ostringstream oss;
    oss << "{"
        << "\"mediaType\":" << static_cast<int>(options.mediaType) << ","
        << "\"copyToSandbox\":" << (options.copyToSandbox ? "true" : "false")
        << "}";
    return oss.str();
}

std::string buildOpenFdArgs(const std::string& fileId) {
    std::ostringstream oss;
    oss << "{"
        << "\"fileId\":\"" << escapeJson(fileId) << "\""
        << "}";
    return oss.str();
}

void fillFileInfo(const LFJSONObject::Ptr& json, LFFileInfo& fileInfo) {
    if (!json) return;
    if (json->contains("fileId")) {
        fileInfo.fileId = json->at("fileId").asString();
    }
    if (json->contains("path")) {
        fileInfo.path = json->at("path").asString();
    }
    if (json->contains("name")) {
        fileInfo.name = json->at("name").asString();
    }
    if (json->contains("mimeType")) {
        fileInfo.mimeType = json->at("mimeType").asString();
    }
    if (json->contains("size")) {
        fileInfo.size = static_cast<int64_t>(json->at("size").asDouble());
    }
}

} // namespace

void LFFilePicker::pickFile(LFFilePickCallback callback) {
    pickFile(LFFilePickerOptions{}, std::move(callback));
}

void LFFilePicker::pickFile(const LFFilePickerOptions& options, LFFilePickCallback callback) {
    if (!callback) return;

    LFNativeSender::getInstance().send(
        "file_picker.pick",
        buildPickArgs(options),
        [callback = std::move(callback)](const LFMethodResult& nativeResult) mutable {
            if (!nativeResult.ok) {
                callback(makePickError(
                    nativeResult.error.empty() ? "pick_failed" : nativeResult.error,
                    nativeResult.canceled
                ));
                return;
            }

            LFFilePickResult pickResult;
            pickResult.ok = true;
            try {
                auto json = LFJSONParser::parse(nativeResult.data);
                fillFileInfo(json, pickResult.file);
            } catch (...) {
                callback(makePickError("pick_result_parse_failed"));
                return;
            }
            callback(pickResult);
        }
    );
}

void LFFilePicker::readFile(const LFFileInfo& file, LFFileReadCallback callback) {
    readFile(file, LFFileReadOptions{}, std::move(callback));
}

void LFFilePicker::readFile(const LFFileInfo& file, const LFFileReadOptions& options, LFFileReadCallback callback) {
    if (!callback) return;

    if (!file.path.empty()) {
        callback(readChunkFromPath(file.path, options));
        return;
    }

    if (file.fileId.empty()) {
        callback(makeReadError("read_source_unavailable"));
        return;
    }

    LFNativeSender::getInstance().send(
        "file_picker.open_fd",
        buildOpenFdArgs(file.fileId),
        [callback = std::move(callback), options](const LFMethodResult& nativeResult) mutable {
            if (!nativeResult.ok) {
                callback(makeReadError(nativeResult.error.empty() ? "open_fd_failed" : nativeResult.error));
                return;
            }

            int fd = -1;
            try {
                auto json = LFJSONParser::parse(nativeResult.data);
                if (json && json->contains("fd")) {
                    fd = json->at("fd").asInt();
                }
            } catch (...) {
                callback(makeReadError("open_fd_result_parse_failed"));
                return;
            }

            if (fd < 0) {
                callback(makeReadError("invalid_fd"));
                return;
            }

#if defined(__ANDROID__) || defined(__APPLE__) || defined(__linux__) || defined(__OHOS__)
            LFFileReadResult result;
            const size_t length = options.length == 0 ? (256 * 1024) : options.length;
            result.bytes.resize(length);
            const off_t offset = static_cast<off_t>(options.offset);
            const ssize_t bytesRead = pread(fd, result.bytes.data(), length, offset);
            close(fd);
            if (bytesRead < 0) {
                callback(makeReadError("read_fd_failed"));
                return;
            }

            result.ok = true;
            result.bytes.resize(static_cast<size_t>(bytesRead));
            result.eof = bytesRead == 0 || static_cast<size_t>(bytesRead) < length;
            callback(result);
#else
            callback(makeReadError("fd_read_not_supported"));
#endif
        }
    );
}
