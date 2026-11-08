//
// Created by Chen Tong on 2026/2/21.
//

#include "LFPathProvider.h"
#include "plugin/LFPlugin.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

namespace {

constexpr const char* kPathUnavailable = "path_unavailable";
constexpr const char* kCreateDirectoryFailed = "create_directory_failed";

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

bool ensureDirectory(const std::filesystem::path& path) {
    if (path.empty()) return false;

    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) return false;
    return std::filesystem::is_directory(path, ec) && !ec;
}

void emitResult(
        const std::filesystem::path& path,
        const std::string& error,
        LFPathProviderCallback callback) {
    if (!callback) return;

    LFPathProviderResult result;
    result.ok = error.empty();
    result.path = result.ok ? asString(path) : "";
    result.error = result.ok ? "" : error;
    LFPluginCenter::dispatchToMain([callback = std::move(callback), result]() mutable {
        callback(result);
    });
}

void emitPath(
        const std::filesystem::path& path,
        LFPathProviderCallback callback) {
    if (path.empty()) {
        emitResult({}, kPathUnavailable, std::move(callback));
        return;
    }
    emitResult(path, "", std::move(callback));
}

void emitError(const std::string& error, LFPathProviderCallback callback) {
    emitResult({}, error, std::move(callback));
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
    return home / "Library" / "Application Support" / "leaf" / LEAF_APPLICATION_ID;
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
    if (path.empty()) {
        emitError(kPathUnavailable, std::move(callback));
        return;
    }
    if (!ensureDirectory(path)) {
        emitError(kCreateDirectoryFailed, std::move(callback));
        return;
    }
    emitPath(path, std::move(callback));
}

void LFPathProvider::getApplicationDocumentsPath(LFPathProviderCallback callback) {
    emitPath(documentsPath(), std::move(callback));
}

void LFPathProvider::getDownloadsPath(LFPathProviderCallback callback) {
    emitPath(downloadsPath(), std::move(callback));
}

void LFPathProvider::getExternalStoragePath(LFPathProviderCallback callback) {
    emitError(kPathUnavailable, std::move(callback));
}
