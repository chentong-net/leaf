//
// Created by Chen Tong on 2026/2/20.
//

#include "LFPathProvider.h"
#include "LFJSONParser.h"
#include "plugin/LFNativeSender.h"

namespace {

LFPathProviderResult makeErrorResult(const std::string& error) {
    LFPathProviderResult result;
    result.ok = false;
    result.error = error;
    return result;
}

void requestPath(const char* method, LFPathProviderCallback callback) {
    if (!callback) return;

    LFNativeSender::getInstance().send(
        method ? method : "",
        "{}",
        [callback = std::move(callback)](const LFMethodResult& nativeResult) mutable {
            if (!nativeResult.ok) {
                callback(makeErrorResult(
                    nativeResult.error.empty() ? "path_provider_failed" : nativeResult.error
                ));
                return;
            }

            try {
                auto json = LFJSONParser::parse(nativeResult.data);
                LFPathProviderResult result;
                result.ok = true;
                if (json && json->contains("path")) {
                    result.path = json->at("path").asString();
                }
                callback(result);
            } catch (...) {
                callback(makeErrorResult("path_result_parse_failed"));
            }
        }
    );
}

} // namespace

void LFPathProvider::getTemporaryPath(LFPathProviderCallback callback) {
    requestPath("path_provider.get_temporary_path", std::move(callback));
}

void LFPathProvider::getApplicationSupportPath(LFPathProviderCallback callback) {
    requestPath("path_provider.get_application_support_path", std::move(callback));
}

void LFPathProvider::getApplicationDocumentsPath(LFPathProviderCallback callback) {
    requestPath("path_provider.get_application_documents_path", std::move(callback));
}

void LFPathProvider::getDownloadsPath(LFPathProviderCallback callback) {
    requestPath("path_provider.get_downloads_path", std::move(callback));
}

void LFPathProvider::getExternalStoragePath(LFPathProviderCallback callback) {
    requestPath("path_provider.get_external_storage_path", std::move(callback));
}
