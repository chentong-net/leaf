//
// Created by Chen Tong on 2026/2/21.
//

#include "LFPathProvider.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

#include <windows.h>

namespace {

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

std::string getenvOrEmpty(const char* name) {
    if (!name) return "";
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string();
}

std::filesystem::path homeDirectory() {
    const std::string home = getenvOrEmpty("USERPROFILE");
    if (!home.empty()) {
        return std::filesystem::path(home);
    }

    const std::string drive = getenvOrEmpty("HOMEDRIVE");
    const std::string path = getenvOrEmpty("HOMEPATH");
    if (!drive.empty() && !path.empty()) {
        return std::filesystem::path(drive + path);
    }
    return {};
}

std::string asString(const std::filesystem::path& path) {
    if (path.empty()) return "";
    return wideToUtf8(path.lexically_normal().wstring());
}

void ensureDirectory(const std::filesystem::path& path) {
    if (path.empty()) return;
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
}

void emitPath(
        const std::filesystem::path& path,
        LFPathProviderCallback callback) {
    if (!callback) return;

    LFPathProviderResult result;
    result.ok = true;
    result.path = asString(path);
    callback(result);
}

std::filesystem::path temporaryPath() {
    std::error_code ec;
    const auto path = std::filesystem::temp_directory_path(ec);
    if (ec) return {};
    return path;
}

std::filesystem::path appSupportPath() {
    const std::string appData = getenvOrEmpty("APPDATA");
    if (!appData.empty()) {
        return std::filesystem::path(appData) / "leaf";
    }

    const auto home = homeDirectory();
    if (home.empty()) return {};
    return home / "AppData" / "Roaming" / "leaf";
}

std::filesystem::path documentsPath() {
    const auto home = homeDirectory();
    if (home.empty()) return {};
    return home / "Documents";
}

std::filesystem::path downloadsPath() {
    const auto home = homeDirectory();
    if (home.empty()) return {};
    return home / "Downloads";
}

} // namespace

void LFPathProvider::getTemporaryPath(LFPathProviderCallback callback) {
    emitPath(temporaryPath(), std::move(callback));
}

void LFPathProvider::getApplicationSupportPath(LFPathProviderCallback callback) {
    const auto path = appSupportPath();
    ensureDirectory(path);
    emitPath(path, std::move(callback));
}

void LFPathProvider::getApplicationDocumentsPath(LFPathProviderCallback callback) {
    emitPath(documentsPath(), std::move(callback));
}

void LFPathProvider::getDownloadsPath(LFPathProviderCallback callback) {
    emitPath(downloadsPath(), std::move(callback));
}

void LFPathProvider::getExternalStoragePath(LFPathProviderCallback callback) {
    emitPath({}, std::move(callback));
}
