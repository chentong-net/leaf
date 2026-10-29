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
    const std::string home = getenvOrEmpty("HOME");
    if (home.empty()) return {};
    return std::filesystem::path(home);
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
    const auto home = homeDirectory();
    if (home.empty()) return {};
    return home / "Library" / "Application Support" / "leaf";
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
