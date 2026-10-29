//
// Created by Chen Tong on 2026/2/18.
//

#include "LFFilePicker.h"

#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>

#include <unistd.h>

namespace {

std::atomic<uint64_t> g_fileIdCounter{1};

std::string stripTrailingNewlines(const std::string& input) {
    size_t end = input.size();
    while (end > 0 && (input[end - 1] == '\n' || input[end - 1] == '\r')) {
        --end;
    }
    return input.substr(0, end);
}

std::string runCommand(const std::string& command) {
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }

    char buffer[512];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    pclose(pipe);
    return stripTrailingNewlines(output);
}

std::string pickMacOSFilePath(LFFilePickerMediaType mediaType) {
    std::string command = "osascript -e 'try' ";
    switch (mediaType) {
        case LFFilePickerMediaType::Image:
            command += "-e 'POSIX path of (choose file with prompt \"Select an image\" of type {\"public.image\"})' ";
            break;
        case LFFilePickerMediaType::Video:
            command += "-e 'POSIX path of (choose file with prompt \"Select a video\" of type {\"public.movie\"})' ";
            break;
        case LFFilePickerMediaType::ImageOrVideo:
            command += "-e 'POSIX path of (choose file with prompt \"Select a media\" of type {\"public.image\", \"public.movie\"})' ";
            break;
        case LFFilePickerMediaType::Any:
        default:
            command += "-e 'POSIX path of (choose file with prompt \"Select a file\")' ";
            break;
    }
    command += "-e 'on error number -128' ";
    command += "-e 'return \"\"' ";
    command += "-e 'end try'";
    return runCommand(command);
}

std::string getMimeType(const std::string& fileName) {
    std::string lowerName;
    lowerName.reserve(fileName.size());
    for (char c : fileName) {
        lowerName += static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }

    auto endsWith = [](const std::string& str, const std::string& suffix) -> bool {
        return str.size() >= suffix.size() &&
               str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    };

    if (endsWith(lowerName, ".jpg") || endsWith(lowerName, ".jpeg")) return "image/jpeg";
    if (endsWith(lowerName, ".png")) return "image/png";
    if (endsWith(lowerName, ".gif")) return "image/gif";
    if (endsWith(lowerName, ".webp")) return "image/webp";
    if (endsWith(lowerName, ".bmp")) return "image/bmp";
    if (endsWith(lowerName, ".heic")) return "image/heic";
    if (endsWith(lowerName, ".heif")) return "image/heif";
    if (endsWith(lowerName, ".svg")) return "image/svg+xml";
    if (endsWith(lowerName, ".ico")) return "image/x-icon";
    if (endsWith(lowerName, ".tiff") || endsWith(lowerName, ".tif")) return "image/tiff";
    if (endsWith(lowerName, ".mp4")) return "video/mp4";
    if (endsWith(lowerName, ".mov")) return "video/quicktime";
    if (endsWith(lowerName, ".mkv")) return "video/x-matroska";
    if (endsWith(lowerName, ".avi")) return "video/x-msvideo";
    if (endsWith(lowerName, ".m4v")) return "video/x-m4v";
    if (endsWith(lowerName, ".3gp")) return "video/3gpp";
    if (endsWith(lowerName, ".webm")) return "video/webm";
    if (endsWith(lowerName, ".mp3")) return "audio/mpeg";
    if (endsWith(lowerName, ".wav")) return "audio/wav";
    if (endsWith(lowerName, ".ogg")) return "audio/ogg";
    if (endsWith(lowerName, ".flac")) return "audio/flac";
    if (endsWith(lowerName, ".pdf")) return "application/pdf";
    if (endsWith(lowerName, ".txt")) return "text/plain";
    if (endsWith(lowerName, ".json")) return "application/json";
    if (endsWith(lowerName, ".html") || endsWith(lowerName, ".htm")) return "text/html";
    if (endsWith(lowerName, ".zip")) return "application/zip";

    return "application/octet-stream";
}

std::string getFileName(const std::string& path) {
    const size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

int64_t getFileSize(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return st.st_size;
    }
    return 0;
}

std::string sanitizeFileName(const std::string& name) {
    if (name.empty()) return "";

    std::string result;
    result.reserve(name.size());
    for (char c : name) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' ||
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            result += '_';
        } else {
            result += c;
        }
    }
    return result;
}

std::string buildUniqueIdSuffix() {
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    return std::to_string(nanos);
}

std::string createSandboxCopyPath(const std::string& selectedPath, bool* ok) {
    if (ok) *ok = false;

    std::filesystem::path tempRoot;
    char tempDir[512];
    if (confstr(_CS_DARWIN_USER_TEMP_DIR, tempDir, sizeof(tempDir)) > 0) {
        tempRoot = std::filesystem::path(tempDir);
    } else {
        tempRoot = std::filesystem::temp_directory_path();
    }

    std::error_code ec;
    const std::filesystem::path sandboxDir = tempRoot / "leaf_file_picker";
    std::filesystem::create_directories(sandboxDir, ec);
    if (ec) {
        return "";
    }

    std::string safeName = sanitizeFileName(getFileName(selectedPath));
    if (safeName.empty()) {
        safeName = "picked_file";
    }

    const std::filesystem::path destPath =
            sandboxDir / (buildUniqueIdSuffix() + "_" + safeName);
    std::filesystem::copy_file(
            std::filesystem::path(selectedPath),
            destPath,
            std::filesystem::copy_options::overwrite_existing,
            ec
    );
    if (ec) {
        return "";
    }

    if (ok) *ok = true;
    return destPath.string();
}

LFFileReadResult readChunkFromPath(
        const std::string& path,
        const LFFileReadOptions& options) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {false, false, {}, "open_path_failed"};
    }

    file.seekg(0, std::ios::end);
    const std::streamoff totalSize = file.tellg();
    if (totalSize < 0) {
        return {false, false, {}, "read_size_failed"};
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
        return {false, false, {}, "read_path_failed"};
    }
    result.bytes.resize(static_cast<size_t>(bytesRead));
    result.eof = static_cast<std::streamoff>(offset + result.bytes.size()) >= totalSize;
    return result;
}

} // namespace

void LFFilePicker::pickFile(LFFilePickCallback callback) {
    pickFile(LFFilePickerOptions{}, std::move(callback));
}

void LFFilePicker::pickFile(
        const LFFilePickerOptions& options,
        LFFilePickCallback callback) {
    if (!callback) return;

    const std::string selectedPath = pickMacOSFilePath(options.mediaType);
    if (selectedPath.empty()) {
        LFFilePickResult result;
        result.ok = true;
        result.canceled = true;
        callback(result);
        return;
    }

    std::string finalPath = selectedPath;
    if (options.copyToSandbox) {
        bool copyOk = false;
        std::string copiedPath = createSandboxCopyPath(selectedPath, &copyOk);
        if (!copyOk || copiedPath.empty()) {
            LFFilePickResult result;
            result.ok = false;
            result.error = "copy_to_sandbox_failed";
            callback(result);
            return;
        }
        finalPath = std::move(copiedPath);
    }

    LFFileInfo fileInfo;
    fileInfo.path = finalPath;
    fileInfo.name = getFileName(selectedPath);
    fileInfo.mimeType = getMimeType(fileInfo.name);
    fileInfo.size = getFileSize(finalPath);

    const uint64_t fileId = g_fileIdCounter.fetch_add(1);
    fileInfo.fileId = "fp_" + std::to_string(fileId);

    LFFilePickResult result;
    result.ok = true;
    result.file = std::move(fileInfo);
    callback(result);
}

void LFFilePicker::readFile(
        const LFFileInfo& file,
        LFFileReadCallback callback) {
    readFile(file, LFFileReadOptions{}, std::move(callback));
}

void LFFilePicker::readFile(
        const LFFileInfo& file,
        const LFFileReadOptions& options,
        LFFileReadCallback callback) {
    if (!callback) return;

    if (!file.path.empty()) {
        callback(readChunkFromPath(file.path, options));
        return;
    }

    if (file.fileId.empty()) {
        callback({false, false, {}, "read_source_unavailable"});
        return;
    }

    callback({false, false, {}, "file_id_not_supported_on_macos"});
}
