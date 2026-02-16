#include "LFFilePicker.h"
#include "plugin/LFPlugin.h"
#include <cstdlib>

// Implemented by platform source:
// - plugins/file_picker/desktop/LFFilePickerDesktop.cpp
// - plugins/file_picker/android/LFFilePickerAndroid.cpp
bool lfFilePickerRequestFromPlatform(int requestId, const LFFilePickerOptions& options, std::string& error);

namespace {

std::mutex g_pendingMutex;
int g_nextRequestId = 1;
std::unordered_map<int, LFPluginCallback> g_pendingCallbacks;

std::string buildParams(const LFFilePickerOptions& options) {
    return std::to_string(static_cast<int>(options.mediaType)) + "|" + (options.copyToSandbox ? "1" : "0");
}

LFFilePickerOptions parseParams(const std::string& params) {
    LFFilePickerOptions options;
    if (params.empty() || params == "{}") {
        return options;
    }

    auto sep = params.find('|');
    if (sep == std::string::npos) {
        return options;
    }

    const std::string typePart = params.substr(0, sep);
    const std::string copyPart = params.substr(sep + 1);

    const int rawType = std::atoi(typePart.c_str());
    if (rawType >= static_cast<int>(LFFilePickerMediaType::Any) &&
        rawType <= static_cast<int>(LFFilePickerMediaType::ImageOrVideo)) {
        options.mediaType = static_cast<LFFilePickerMediaType>(rawType);
    }
    options.copyToSandbox = std::atoi(copyPart.c_str()) != 0;
    return options;
}

void resolveRequest(int requestId, const LFPluginResult& result) {
    LFPluginCallback callback;
    {
        std::lock_guard<std::mutex> lock(g_pendingMutex);
        auto it = g_pendingCallbacks.find(requestId);
        if (it != g_pendingCallbacks.end()) {
            callback = std::move(it->second);
            g_pendingCallbacks.erase(it);
        }
    }

    if (!callback) {
        return;
    }

    LFPluginCenter::dispatchToMain([callback, result]() {
        callback(result);
    });
}

void invokeFilePicker(const std::string& method, const std::string& params, LFPluginCallback callback) {
    if (method != "pick") {
        LFPluginResult result;
        result.ok = false;
        result.error = "method_not_supported";
        result.code = -2;
        LFPluginCenter::dispatchToMain([callback, result]() {
            if (callback) callback(result);
        });
        return;
    }

    if (!callback) {
        return;
    }

    int requestId = 0;
    {
        std::lock_guard<std::mutex> lock(g_pendingMutex);
        requestId = g_nextRequestId++;
        g_pendingCallbacks[requestId] = std::move(callback);
    }

    const LFFilePickerOptions options = parseParams(params);

    std::string error;
    if (!lfFilePickerRequestFromPlatform(requestId, options, error)) {
        LFPluginResult result;
        result.ok = false;
        result.error = error.empty() ? "platform_request_failed" : error;
        result.code = -3;
        resolveRequest(requestId, result);
    }
}

void ensureFilePickerRegistered() {
    static bool s_registered = []() {
        LFPluginCenter::getInstance().registerPlugin("file_picker", invokeFilePicker);
        return true;
    }();
    (void) s_registered;
}

struct LFFilePickerAutoRegister {
    LFFilePickerAutoRegister() {
        ensureFilePickerRegistered();
    }
} g_autoRegister;

}

void LFFilePicker::pickFile(LFFilePickerCallback callback) {
    pickFile(LFFilePickerOptions{}, std::move(callback));
}

void LFFilePicker::pickFile(const LFFilePickerOptions& options, LFFilePickerCallback callback) {
    ensureFilePickerRegistered();

    LFPluginCenter::getInstance().invoke("file_picker", "pick", buildParams(options), [callback](const LFPluginResult& result) {
        if (!callback) {
            return;
        }

        LFFilePickerResult pickerResult;
        pickerResult.ok = result.ok;
        if (result.ok) {
            pickerResult.path = result.data;
        } else {
            pickerResult.error = result.error;
            pickerResult.canceled = (result.error == "canceled");
        }
        callback(pickerResult);
    });
}

extern "C" void lfFilePickerOnPlatformResult(
        int requestId,
        int success,
        const char* path,
        const char* error) {
    LFPluginResult result;
    result.ok = success != 0;
    if (result.ok) {
        if (path) {
            result.data = path;
        }
    } else {
        result.error = error ? error : "unknown_error";
        result.code = (result.error == "canceled") ? 1 : -4;
    }

    resolveRequest(requestId, result);
}
