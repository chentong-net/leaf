//
// Created by Chen Tong on 2026/2/18.
//

#include "LFFilePicker.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sys/stat.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#include <unistd.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

std::string trim(const std::string& input) {
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }

    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
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
    return trim(output);
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
    size_t pos = path.find_last_of("/\\");
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

} // namespace

void LFFilePicker::pickFile(LFFilePickCallback callback) {
    pickFile(LFFilePickerOptions{}, std::move(callback));
}

void LFFilePicker::pickFile(const LFFilePickerOptions& options, LFFilePickCallback callback) {
    if (!callback) return;

    std::string command;

#if defined(__APPLE__)
    command = "osascript -e 'try' ";
    switch (options.mediaType) {
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
#elif defined(_WIN32)
    command = "powershell -Command \"";
    command += "Add-Type -AssemblyName System.Windows.Forms; ";
    command += "$dialog = New-Object System.Windows.Forms.OpenFileDialog; ";

    switch (options.mediaType) {
        case LFFilePickerMediaType::Image:
            command += "$dialog.Filter = 'Images|*.jpg;*.jpeg;*.png;*.gif;*.bmp;*.webp;*.heic;*.heif|All Files|*.*'; ";
            break;
        case LFFilePickerMediaType::Video:
            command += "$dialog.Filter = 'Videos|*.mp4;*.mov;*.mkv;*.avi;*.m4v;*.3gp;*.webm|All Files|*.*'; ";
            break;
        case LFFilePickerMediaType::ImageOrVideo:
            command += "$dialog.Filter = 'Media|*.jpg;*.jpeg;*.png;*.gif;*.bmp;*.webp;*.heic;*.heif;*.mp4;*.mov;*.mkv;*.avi;*.m4v;*.3gp;*.webm|All Files|*.*'; ";
            break;
        case LFFilePickerMediaType::Any:
        default:
            command += "$dialog.Filter = 'All Files|*.*'; ";
            break;
    }

    command += "$dialog.Title = 'Select a File'; ";
    command += "if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) { $dialog.FileName } else { '' }";
    command += "\"";
#else
    LFFilePickResult result;
    result.ok = false;
    result.error = "platform_not_supported";
    callback(result);
    return;
#endif

    std::string selectedPath = runCommand(command);

    if (selectedPath.empty()) {
        LFFilePickResult result;
        result.ok = true;
        result.canceled = true;
        callback(result);
        return;
    }

    std::string finalPath = selectedPath;
    if (options.copyToSandbox) {
        char tempDir[512];
#if defined(__APPLE__)
        if (confstr(_CS_DARWIN_USER_TEMP_DIR, tempDir, sizeof(tempDir)) <= 0) {
            strcpy(tempDir, "/tmp");
        }
#elif defined(_WIN32)
        DWORD len = GetTempPathA(sizeof(tempDir), tempDir);
        if (len == 0) {
            strcpy(tempDir, "C:\\Temp");
        }
#else
        strcpy(tempDir, "/tmp");
#endif

        std::string sandboxDir = std::string(tempDir) + "leaf_file_picker";
        std::string mkdirCmd;
#if defined(_WIN32)
        mkdirCmd = "mkdir \"" + sandboxDir + "\" 2>nul";
#else
        mkdirCmd = "mkdir -p \"" + sandboxDir + "\"";
#endif
        system(mkdirCmd.c_str());

        std::string fileName = getFileName(selectedPath);
        std::string safeName = sanitizeFileName(fileName);
        if (safeName.empty()) {
            safeName = "picked_file";
        }

        char timestamp[64];
        snprintf(timestamp, sizeof(timestamp), "%lld", (long long)time(nullptr));

        std::string destPath = sandboxDir + "/" + timestamp + "_" + safeName;

        std::string copyCmd;
#if defined(_WIN32)
        copyCmd = "copy \"" + selectedPath + "\" \"" + destPath + "\" >nul 2>&1";
#else
        copyCmd = "cp \"" + selectedPath + "\" \"" + destPath + "\"";
#endif
        int copyResult = system(copyCmd.c_str());

        if (copyResult == 0) {
            finalPath = destPath;
        }
    }

    LFFileInfo fileInfo;
    fileInfo.path = finalPath;
    fileInfo.name = getFileName(finalPath);
    fileInfo.mimeType = getMimeType(fileInfo.name);
    fileInfo.size = getFileSize(finalPath);

    static int fileIdCounter = 1;
    char fileId[64];
    snprintf(fileId, sizeof(fileId), "fp_%d", fileIdCounter++);
    fileInfo.fileId = fileId;

    LFFilePickResult result;
    result.ok = true;
    result.file = fileInfo;
    callback(result);
}

LFFileReadResult readChunkFromPath(const std::string& path, const LFFileReadOptions& options) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        LFFileReadResult result;
        result.ok = false;
        result.error = "open_path_failed";
        return result;
    }

    file.seekg(0, std::ios::end);
    const std::streamoff totalSize = file.tellg();
    if (totalSize < 0) {
        LFFileReadResult result;
        result.ok = false;
        result.error = "read_size_failed";
        return result;
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
        LFFileReadResult result;
        result.ok = false;
        result.error = "read_path_failed";
        return result;
    }
    result.bytes.resize(static_cast<size_t>(bytesRead));
    result.eof = static_cast<std::streamoff>(offset + result.bytes.size()) >= totalSize;
    return result;
}

void LFFilePicker::readFile(const LFFileInfo& file, LFFileReadCallback callback) {
    readFile(file, LFFileReadOptions{}, std::move(callback));
}

void LFFilePicker::readFile(const LFFileInfo& file, const LFFileReadOptions& options, LFFileReadCallback callback) {
    if (!callback) return;

    if (!file.path.empty()) {
        LFFileReadResult result = readChunkFromPath(file.path, options);
        callback(result);
        return;
    }

    if (file.fileId.empty()) {
        LFFileReadResult result;
        result.ok = false;
        result.error = "read_source_unavailable";
        callback(result);
        return;
    }

    LFFileReadResult result;
    result.ok = false;
    result.error = "file_id_not_supported_on_desktop";
    callback(result);
}
