#include "LFFileService.h"
#include "plugin/LFPlugin.h"

namespace {

std::mutex g_fileServiceMutex;
std::shared_ptr<LFFileService> g_fileService;

std::shared_ptr<LFFileService> getFileService() {
    std::lock_guard<std::mutex> lock(g_fileServiceMutex);
    return g_fileService;
}

void dispatchUnsupportedPick(LFFilePickCallback callback) {
    if (!callback) {
        return;
    }
    LFFilePickResult result;
    result.ok = false;
    result.error = "not_supported";
    LFPluginCenter::dispatchToMain([callback, result]() {
        callback(result);
    });
}

void dispatchUnsupportedRead(LFFileReadCallback callback) {
    if (!callback) {
        return;
    }
    LFFileReadResult result;
    result.ok = false;
    result.error = "not_supported";
    LFPluginCenter::dispatchToMain([callback, result]() {
        callback(result);
    });
}

void dispatchUnsupportedSave(LFFileSaveCallback callback) {
    if (!callback) {
        return;
    }
    LFFileSaveResult result;
    result.ok = false;
    result.error = "not_supported";
    LFPluginCenter::dispatchToMain([callback, result]() {
        callback(result);
    });
}

} // namespace

void LFFileSystem::setFileService(const std::shared_ptr<LFFileService>& service) {
    std::lock_guard<std::mutex> lock(g_fileServiceMutex);
    g_fileService = service;
}

void LFFileSystem::pickFile(const LFFilePickOptions& options, LFFilePickCallback callback) {
    auto service = getFileService();
    if (!service) {
        dispatchUnsupportedPick(std::move(callback));
        return;
    }
    service->pickFile(options, std::move(callback));
}

void LFFileSystem::readFile(const std::string& fileId, LFFileReadCallback callback) {
    auto service = getFileService();
    if (!service) {
        dispatchUnsupportedRead(std::move(callback));
        return;
    }
    service->readFile(fileId, std::move(callback));
}

void LFFileSystem::saveFile(const LFFileSaveOptions& options, const std::string& content, LFFileSaveCallback callback) {
    auto service = getFileService();
    if (!service) {
        dispatchUnsupportedSave(std::move(callback));
        return;
    }
    service->saveFile(options, content, std::move(callback));
}
