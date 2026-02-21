//
// Created by Chen Tong on 2026/2/20.
//

#include "LFPathProvider.h"

#include <emscripten/em_js.h>

namespace {

EM_JS(void, leafWebEnsureDir, (const char* pathPtr), {
    const path = UTF8ToString(pathPtr);
    if (!path || path.length === 0) return;
    const parts = path.split('/').filter(Boolean);
    let current = "";
    for (const part of parts) {
        current += '/' + part;
        try {
            FS.mkdir(current);
        } catch (e) {
            if (!e || e.errno !== 20) {
                throw e;
            }
        }
    }
});

void emitPath(const std::string& path, LFPathProviderCallback callback) {
    if (!callback) return;
    LFPathProviderResult result;
    result.ok = true;
    result.path = path;
    callback(result);
}

} // namespace

void LFPathProvider::getTemporaryPath(LFPathProviderCallback callback) {
    emitPath("/tmp", std::move(callback));
}

void LFPathProvider::getApplicationSupportPath(LFPathProviderCallback callback) {
    const char* path = "/leaf/app_support";
    leafWebEnsureDir(path);
    emitPath(path, std::move(callback));
}

void LFPathProvider::getApplicationDocumentsPath(LFPathProviderCallback callback) {
    const char* path = "/leaf/documents";
    leafWebEnsureDir(path);
    emitPath(path, std::move(callback));
}

void LFPathProvider::getDownloadsPath(LFPathProviderCallback callback) {
    emitPath("", std::move(callback));
}

void LFPathProvider::getExternalStoragePath(LFPathProviderCallback callback) {
    emitPath("", std::move(callback));
}
