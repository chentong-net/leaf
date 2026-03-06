//
// Created by Chen Tong on 2026/2/18.
//

#include "LFFilePicker.h"

#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>

#if defined(__APPLE__)
#include <unistd.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#include <shobjidl.h>
#endif

namespace {

std::atomic<uint64_t> g_fileIdCounter{1};

#if defined(_WIN32)
std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";

    const int wideCount = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        utf8.c_str(),
        static_cast<int>(utf8.size()),
        nullptr,
        0
    );
    if (wideCount <= 0) {
        return L"";
    }

    std::wstring wide(static_cast<size_t>(wideCount), L'\0');
    const int converted = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        utf8.c_str(),
        static_cast<int>(utf8.size()),
        wide.data(),
        wideCount
    );
    if (converted <= 0) {
        return L"";
    }
    return wide;
}

std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return "";

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
        return "";
    }

    std::string utf8(static_cast<size_t>(utf8Count), '\0');
    const int converted = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        utf8.data(),
        utf8Count,
        nullptr,
        nullptr
    );
    if (converted <= 0) {
        return "";
    }
    return utf8;
}

bool pickWindowsFilePath(LFFilePickerMediaType mediaType, std::string* outPath, bool* canceled, std::string* error) {
    if (outPath) outPath->clear();
    if (canceled) *canceled = false;
    if (error) error->clear();

    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool shouldUninit = SUCCEEDED(initHr);
    if (FAILED(initHr) && initHr != RPC_E_CHANGED_MODE) {
        if (error) *error = "com_initialize_failed";
        return false;
    }

    IFileOpenDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (FAILED(hr) || dialog == nullptr) {
        if (shouldUninit) CoUninitialize();
        if (error) *error = "create_file_dialog_failed";
        return false;
    }

    DWORD dialogOptions = 0;
    if (SUCCEEDED(dialog->GetOptions(&dialogOptions))) {
        dialog->SetOptions(dialogOptions | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
    }
    dialog->SetTitle(L"Select a File");

    COMDLG_FILTERSPEC filters[2];
    UINT filterCount = 0;
    switch (mediaType) {
        case LFFilePickerMediaType::Image:
            filters[0] = {L"Images", L"*.jpg;*.jpeg;*.png;*.gif;*.bmp;*.webp;*.heic;*.heif"};
            filters[1] = {L"All Files", L"*.*"};
            filterCount = 2;
            break;
        case LFFilePickerMediaType::Video:
            filters[0] = {L"Videos", L"*.mp4;*.mov;*.mkv;*.avi;*.m4v;*.3gp;*.webm"};
            filters[1] = {L"All Files", L"*.*"};
            filterCount = 2;
            break;
        case LFFilePickerMediaType::ImageOrVideo:
            filters[0] = {L"Media", L"*.jpg;*.jpeg;*.png;*.gif;*.bmp;*.webp;*.heic;*.heif;*.mp4;*.mov;*.mkv;*.avi;*.m4v;*.3gp;*.webm"};
            filters[1] = {L"All Files", L"*.*"};
            filterCount = 2;
            break;
        case LFFilePickerMediaType::Any:
        default:
            filters[0] = {L"All Files", L"*.*"};
            filterCount = 1;
            break;
    }
    if (filterCount > 0) {
        dialog->SetFileTypes(filterCount, filters);
        dialog->SetFileTypeIndex(1);
    }

    hr = dialog->Show(nullptr);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        dialog->Release();
        if (shouldUninit) CoUninitialize();
        if (canceled) *canceled = true;
        return true;
    }
    if (FAILED(hr)) {
        dialog->Release();
        if (shouldUninit) CoUninitialize();
        if (error) *error = "show_file_dialog_failed";
        return false;
    }

    IShellItem* item = nullptr;
    hr = dialog->GetResult(&item);
    if (FAILED(hr) || item == nullptr) {
        dialog->Release();
        if (shouldUninit) CoUninitialize();
        if (error) *error = "get_dialog_result_failed";
        return false;
    }

    PWSTR widePath = nullptr;
    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &widePath);
    if (FAILED(hr) || widePath == nullptr) {
        item->Release();
        dialog->Release();
        if (shouldUninit) CoUninitialize();
        if (error) *error = "get_selected_path_failed";
        return false;
    }

    const std::wstring pathWide(widePath);
    CoTaskMemFree(widePath);
    item->Release();
    dialog->Release();
    if (shouldUninit) CoUninitialize();

    const std::string pathUtf8 = wideToUtf8(pathWide);
    if (pathUtf8.empty()) {
        if (error) *error = "path_convert_failed";
        return false;
    }

    if (outPath) *outPath = pathUtf8;
    return true;
}
#endif

std::string stripTrailingNewlines(const std::string& input) {
    size_t end = input.size();
    while (end > 0 && (input[end - 1] == '\n' || input[end - 1] == '\r')) {
        --end;
    }
    return input.substr(0, end);
}

std::string runCommand(const std::string& command) {
#if defined(_WIN32)
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (!pipe) {
        return "";
    }

    char buffer[512];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return stripTrailingNewlines(output);
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
#if defined(_WIN32)
    const std::wstring widePath = utf8ToWide(path);
    if (widePath.empty()) {
        return 0;
    }

    struct _stat64 st;
    if (_wstat64(widePath.c_str(), &st) == 0) {
        return st.st_size;
    }
    return 0;
#else
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return st.st_size;
    }
    return 0;
#endif
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
    namespace fs = std::filesystem;

    fs::path tempRoot;
#if defined(__APPLE__)
    char tempDir[512];
    if (confstr(_CS_DARWIN_USER_TEMP_DIR, tempDir, sizeof(tempDir)) > 0) {
        tempRoot = fs::path(tempDir);
    } else {
        tempRoot = fs::temp_directory_path();
    }
#elif defined(_WIN32)
    wchar_t tempDir[MAX_PATH];
    DWORD len = GetTempPathW(MAX_PATH, tempDir);
    if (len > 0) {
        tempRoot = fs::path(tempDir);
    } else {
        tempRoot = fs::temp_directory_path();
    }
#else
    tempRoot = fs::temp_directory_path();
#endif

    std::error_code ec;
    fs::path sandboxDir = tempRoot / "leaf_file_picker";
    fs::create_directories(sandboxDir, ec);
    if (ec) {
        return "";
    }

    std::string safeName = sanitizeFileName(getFileName(selectedPath));
    if (safeName.empty()) {
        safeName = "picked_file";
    }

#if defined(_WIN32)
    const std::wstring sourceWide = utf8ToWide(selectedPath);
    const std::wstring destNameWide = utf8ToWide(buildUniqueIdSuffix() + "_" + safeName);
    if (sourceWide.empty() || destNameWide.empty()) {
        return "";
    }

    fs::path sourcePath(sourceWide);
    fs::path destPath = sandboxDir / fs::path(destNameWide);
    fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        return "";
    }

    if (ok) *ok = true;
    return wideToUtf8(destPath.wstring());
#else
    fs::path destPath = sandboxDir / (buildUniqueIdSuffix() + "_" + safeName);
    fs::copy_file(fs::path(selectedPath), destPath, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        return "";
    }

    if (ok) *ok = true;
    return destPath.string();
#endif
}

} // namespace

void LFFilePicker::pickFile(LFFilePickCallback callback) {
    pickFile(LFFilePickerOptions{}, std::move(callback));
}

void LFFilePicker::pickFile(const LFFilePickerOptions& options, LFFilePickCallback callback) {
    if (!callback) return;

    std::string selectedPath;

#if defined(__APPLE__)
    std::string command;
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

    selectedPath = runCommand(command);
    if (selectedPath.empty()) {
        LFFilePickResult result;
        result.ok = true;
        result.canceled = true;
        callback(result);
        return;
    }
#elif defined(_WIN32)
    bool canceled = false;
    std::string pickError;
    if (!pickWindowsFilePath(options.mediaType, &selectedPath, &canceled, &pickError)) {
        LFFilePickResult result;
        result.ok = false;
        result.error = pickError.empty() ? "pick_failed" : pickError;
        callback(result);
        return;
    }

    if (canceled || selectedPath.empty()) {
        LFFilePickResult result;
        result.ok = true;
        result.canceled = true;
        callback(result);
        return;
    }
#else
    LFFilePickResult result;
    result.ok = false;
    result.error = "platform_not_supported";
    callback(result);
    return;
#endif

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
        finalPath = copiedPath;
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
    result.file = fileInfo;
    callback(result);
}

LFFileReadResult readChunkFromPath(const std::string& path, const LFFileReadOptions& options) {
#if defined(_WIN32)
    std::ifstream file(std::filesystem::path(utf8ToWide(path)), std::ios::binary);
#else
    std::ifstream file(path, std::ios::binary);
#endif
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
