//
// Created by Chen Tong on 2026/2/21.
//

#include "LFPathProvider.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

namespace {

std::string getenvOrEmpty(const char* name) {
    if (!name) return "";
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string();
}

std::filesystem::path homeDirectory() {
#if defined(_WIN32)
    std::string home = getenvOrEmpty("USERPROFILE");
    if (!home.empty()) return std::filesystem::path(home);
    std::string drive = getenvOrEmpty("HOMEDRIVE");
    std::string path = getenvOrEmpty("HOMEPATH");
    if (!drive.empty() && !path.empty()) {
        return std::filesystem::path(drive + path);
    }
    return std::filesystem::path();
#else
    std::string home = getenvOrEmpty("HOME");
    if (!home.empty()) return std::filesystem::path(home);
    return std::filesystem::path();
#endif
}

std::string asString(const std::filesystem::path& path) {
    if (path.empty()) return "";
    return path.lexically_normal().string();
}

void ensureDirectory(const std::filesystem::path& path) {
    if (path.empty()) return;
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
}

void emitPath(const std::filesystem::path& path, LFPathProviderCallback callback) {
    if (!callback) return;
    LFPathProviderResult result;
    result.ok = true;
    result.path = asString(path);
    callback(result);
}

std::filesystem::path temporaryPath() {
    std::error_code ec;
    auto path = std::filesystem::temp_directory_path(ec);
    if (ec) return {};
    return path;
}

std::filesystem::path appSupportPath() {
#if defined(_WIN32)
    std::string appData = getenvOrEmpty("APPDATA");
    if (!appData.empty()) {
        return std::filesystem::path(appData) / "leaf";
    }
    auto home = homeDirectory();
    if (home.empty()) return {};
    return home / "AppData" / "Roaming" / "leaf";
#elif defined(__APPLE__)
    auto home = homeDirectory();
    if (home.empty()) return {};
    return home / "Library" / "Application Support" / "leaf";
#else
    std::string xdgData = getenvOrEmpty("XDG_DATA_HOME");
    if (!xdgData.empty()) {
        return std::filesystem::path(xdgData) / "leaf";
    }
    auto home = homeDirectory();
    if (home.empty()) return {};
    return home / ".local" / "share" / "leaf";
#endif
}

std::filesystem::path documentsPath() {
    auto home = homeDirectory();
    if (home.empty()) return {};
    return home / "Documents";
}

std::filesystem::path downloadsPath() {
    auto home = homeDirectory();
    if (home.empty()) return {};
    return home / "Downloads";
}

} // namespace

void LFPathProvider::getTemporaryPath(LFPathProviderCallback callback) {
    emitPath(temporaryPath(), std::move(callback));
}

void LFPathProvider::getApplicationSupportPath(LFPathProviderCallback callback) {
    auto path = appSupportPath();
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

