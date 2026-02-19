//
// Created by Chen Tong on 2026/2/19.
//

#include "LFFilePicker.h"

#include <emscripten/em_js.h>
#include <emscripten/emscripten.h>

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <unordered_map>

namespace {

enum class WebPickErrorCode : int {
    None = 0,
    PickInProgress = 1,
    PickFailed = 2,
    CopyToSandboxFailed = 3,
    MetadataUnavailable = 4
};

enum class WebReadErrorCode : int {
    None = 0,
    FileNotFound = 1,
    ReadFailed = 2
};

std::atomic<int> g_requestId{1};
std::mutex g_mutex;
std::unordered_map<int, LFFilePickCallback> g_pendingPickCallbacks;
std::unordered_map<int, LFFileReadCallback> g_pendingReadCallbacks;
std::unordered_map<std::string, int> g_fileIdToHandle;

std::string mapPickError(WebPickErrorCode code) {
    switch (code) {
        case WebPickErrorCode::PickInProgress:
            return "pick_in_progress";
        case WebPickErrorCode::CopyToSandboxFailed:
            return "copy_to_sandbox_failed";
        case WebPickErrorCode::MetadataUnavailable:
            return "pick_result_parse_failed";
        case WebPickErrorCode::PickFailed:
        case WebPickErrorCode::None:
        default:
            return "pick_failed";
    }
}

std::string mapReadError(WebReadErrorCode code) {
    switch (code) {
        case WebReadErrorCode::FileNotFound:
            return "read_source_unavailable";
        case WebReadErrorCode::ReadFailed:
        case WebReadErrorCode::None:
        default:
            return "read_file_failed";
    }
}

std::string takeJsString(char* raw) {
    if (!raw) return "";
    std::string out(raw);
    std::free(raw);
    return out;
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
        LFFileReadResult failed;
        failed.ok = false;
        failed.error = "read_path_failed";
        return failed;
    }

    result.bytes.resize(static_cast<size_t>(bytesRead));
    result.eof = static_cast<std::streamoff>(offset + result.bytes.size()) >= totalSize;
    return result;
}

EM_JS(void, leafWebStartPick, (int requestId, int mediaType, int copyToSandbox), {
    const getState = () => {
        if (!Module.__leafFilePickerState) {
            Module.__leafFilePickerState = {
                picking: false,
                nextHandle: 1,
                filesByHandle: new Map(),
                handleByFileId: new Map(),
                readBufferPtr: 0,
                readBufferCap: 0
            };
        }
        return Module.__leafFilePickerState;
    };

    const state = getState();
    if (state.picking) {
        Module._leafWebFilePickerOnPickResult(requestId, 0, 0, 0, 1);
        return;
    }
    state.picking = true;

    const ensureDir = (dirPath) => {
        const parts = dirPath.split("/").filter(Boolean);
        let current = "";
        for (const part of parts) {
            current += "/" + part;
            try {
                FS.mkdir(current);
            } catch (e) {
                if (!e || e.errno !== 20) {
                    throw e;
                }
            }
        }
    };

    const sanitize = (name) => {
        if (!name) return "picked_file";
        let out = "";
        for (let i = 0; i < name.length; i++) {
            const code = name.charCodeAt(i);
            // \ / : * ? " < > |
            if (code === 92 || code === 47 || code === 58 || code === 42 ||
                code === 63 || code === 34 || code === 60 || code === 62 || code === 124) {
                out += "_";
            } else {
                out += name[i];
            }
        }
        return out;
    };

    const acceptByMediaType = (type) => {
        switch (type) {
            case 1:
                return "image/*";
            case 2:
                return "video/*";
            case 3:
                return "image/*,video/*";
            default:
                return "";
        }
    };

    const input = document.createElement("input");
    input.type = "file";
    input.style.display = "none";
    const accept = acceptByMediaType(mediaType);
    if (accept) {
        input.accept = accept;
    }

    const cleanup = () => {
        state.picking = false;
        if (input.parentNode) {
            input.parentNode.removeChild(input);
        }
    };

    const finishWithError = (code) => {
        Module._leafWebFilePickerOnPickResult(requestId, 0, 0, 0, code);
        cleanup();
    };

    const finishCanceled = () => {
        Module._leafWebFilePickerOnPickResult(requestId, 0, 1, 0, 0);
        cleanup();
    };

    input.onchange = async () => {
        try {
            const file = input.files && input.files.length > 0 ? input.files[0] : null;
            if (!file) {
                finishCanceled();
                return;
            }

            let sandboxPath = "";
            if (copyToSandbox) {
                try {
                    ensureDir("/tmp/leaf/file_picker");
                    const safeName = sanitize(file.name);
                    const unique = Date.now().toString() + "_" + Math.floor(Math.random() * 1000000).toString();
                    sandboxPath = "/tmp/leaf/file_picker/" + unique + "_" + safeName;
                    const buffer = await file.arrayBuffer();
                    FS.writeFile(sandboxPath, new Uint8Array(buffer));
                } catch (e) {
                    finishWithError(3);
                    return;
                }
            }

            const handle = state.nextHandle++;
            const fileId = "fp_" + handle.toString();
            state.filesByHandle.set(handle, {
                file,
                fileId,
                path: sandboxPath,
                name: file.name || "",
                mimeType: file.type || "",
                size: Number(file.size || 0)
            });
            state.handleByFileId.set(fileId, handle);

            Module._leafWebFilePickerOnPickResult(requestId, 1, 0, handle, 0);
            cleanup();
        } catch (e) {
            finishWithError(2);
        }
    };

    input.oncancel = () => {
        finishCanceled();
    };

    document.body.appendChild(input);
    try {
        input.click();
    } catch (e) {
        finishWithError(2);
    }
});

EM_JS(int, leafWebLookupHandleByFileId, (const char* fileIdPtr), {
    const state = Module.__leafFilePickerState;
    if (!state || !state.handleByFileId) return 0;
    const fileId = UTF8ToString(fileIdPtr);
    const handle = state.handleByFileId.get(fileId);
    return handle ? handle : 0;
});

EM_JS(char*, leafWebGetFileStringField, (int handle, int field), {
    const state = Module.__leafFilePickerState;
    const record = state && state.filesByHandle ? state.filesByHandle.get(handle) : null;
    if (!record) {
        return stringToNewUTF8("");
    }

    let value = "";
    switch (field) {
        case 0:
            value = record.fileId || "";
            break;
        case 1:
            value = record.path || "";
            break;
        case 2:
            value = record.name || "";
            break;
        case 3:
            value = record.mimeType || "";
            break;
        default:
            value = "";
            break;
    }
    return stringToNewUTF8(value);
});

EM_JS(double, leafWebGetFileSize, (int handle), {
    const state = Module.__leafFilePickerState;
    const record = state && state.filesByHandle ? state.filesByHandle.get(handle) : null;
    if (!record) return 0;
    return Number(record.size || 0);
});

EM_JS(void, leafWebReadChunk, (int requestId, int handle, double offset, double length), {
    const state = Module.__leafFilePickerState;
    const record = state && state.filesByHandle ? state.filesByHandle.get(handle) : null;
    if (!record || !record.file) {
        Module._leafWebFilePickerOnReadResult(requestId, 0, 0, 0, 0, 1);
        return;
    }

    const file = record.file;
    const start = Math.max(0, Math.floor(Number(offset)));
    const size = Number(file.size || 0);
    const requestLen = Math.max(0, Math.floor(Number(length)));
    if (start >= size) {
        Module._leafWebFilePickerOnReadResult(requestId, 1, 1, 0, 0, 0);
        return;
    }

    const end = Math.min(size, start + requestLen);
    file.slice(start, end).arrayBuffer().then((buffer) => {
        const bytes = new Uint8Array(buffer);
        const len = bytes.length;
        let ptr = 0;
        if (len > 0) {
            if (!state.readBufferPtr || state.readBufferCap < len) {
                state.readBufferPtr = _malloc(len);
                state.readBufferCap = len;
            }
            ptr = state.readBufferPtr;
            HEAPU8.set(bytes, ptr);
        }
        Module._leafWebFilePickerOnReadResult(requestId, 1, end >= size ? 1 : 0, ptr, len, 0);
    }).catch(() => {
        Module._leafWebFilePickerOnReadResult(requestId, 0, 0, 0, 0, 2);
    });
});

} // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE
void leafWebFilePickerOnPickResult(int requestId, int ok, int canceled, int handle, int errorCode) {
    LFFilePickCallback callback;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_pendingPickCallbacks.find(requestId);
        if (it == g_pendingPickCallbacks.end()) {
            return;
        }
        callback = std::move(it->second);
        g_pendingPickCallbacks.erase(it);
    }

    if (!callback) {
        return;
    }

    LFFilePickResult result;
    if (canceled != 0) {
        result.ok = false;
        result.canceled = true;
        callback(result);
        return;
    }

    if (ok == 0) {
        result.ok = false;
        result.error = mapPickError(static_cast<WebPickErrorCode>(errorCode));
        callback(result);
        return;
    }

    if (handle <= 0) {
        result.ok = false;
        result.error = mapPickError(WebPickErrorCode::MetadataUnavailable);
        callback(result);
        return;
    }

    result.ok = true;
    result.file.fileId = takeJsString(leafWebGetFileStringField(handle, 0));
    result.file.path = takeJsString(leafWebGetFileStringField(handle, 1));
    result.file.name = takeJsString(leafWebGetFileStringField(handle, 2));
    result.file.mimeType = takeJsString(leafWebGetFileStringField(handle, 3));
    result.file.size = static_cast<int64_t>(leafWebGetFileSize(handle));

    if (result.file.fileId.empty()) {
        result.ok = false;
        result.error = mapPickError(WebPickErrorCode::MetadataUnavailable);
        callback(result);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_fileIdToHandle[result.file.fileId] = handle;
    }

    callback(result);
}

EMSCRIPTEN_KEEPALIVE
void leafWebFilePickerOnReadResult(
    int requestId,
    int ok,
    int eof,
    unsigned char* data,
    int dataLen,
    int errorCode
) {
    LFFileReadCallback callback;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_pendingReadCallbacks.find(requestId);
        if (it == g_pendingReadCallbacks.end()) {
            return;
        }
        callback = std::move(it->second);
        g_pendingReadCallbacks.erase(it);
    }

    if (!callback) {
        return;
    }

    LFFileReadResult result;
    if (ok == 0) {
        result.ok = false;
        result.error = mapReadError(static_cast<WebReadErrorCode>(errorCode));
        callback(result);
        return;
    }

    result.ok = true;
    result.eof = eof != 0;
    if (data && dataLen > 0) {
        result.bytes.assign(data, data + dataLen);
    }
    callback(result);
}

} // extern "C"

void LFFilePicker::pickFile(LFFilePickCallback callback) {
    pickFile(LFFilePickerOptions{}, std::move(callback));
}

void LFFilePicker::pickFile(const LFFilePickerOptions& options, LFFilePickCallback callback) {
    if (!callback) return;

    const int requestId = g_requestId.fetch_add(1);
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_pendingPickCallbacks[requestId] = std::move(callback);
    }

    leafWebStartPick(requestId, static_cast<int>(options.mediaType), options.copyToSandbox ? 1 : 0);
}

void LFFilePicker::readFile(const LFFileInfo& file, LFFileReadCallback callback) {
    readFile(file, LFFileReadOptions{}, std::move(callback));
}

void LFFilePicker::readFile(
    const LFFileInfo& file,
    const LFFileReadOptions& options,
    LFFileReadCallback callback
) {
    if (!callback) return;

    if (!file.path.empty()) {
        callback(readChunkFromPath(file.path, options));
        return;
    }

    if (file.fileId.empty()) {
        LFFileReadResult result;
        result.ok = false;
        result.error = "read_source_unavailable";
        callback(result);
        return;
    }

    int handle = 0;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_fileIdToHandle.find(file.fileId);
        if (it != g_fileIdToHandle.end()) {
            handle = it->second;
        }
    }

    if (handle <= 0) {
        handle = leafWebLookupHandleByFileId(file.fileId.c_str());
        if (handle > 0) {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_fileIdToHandle[file.fileId] = handle;
        }
    }

    if (handle <= 0) {
        LFFileReadResult result;
        result.ok = false;
        result.error = "read_source_unavailable";
        callback(result);
        return;
    }

    const int requestId = g_requestId.fetch_add(1);
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_pendingReadCallbacks[requestId] = std::move(callback);
    }

    const size_t requestLength = options.length == 0 ? (256 * 1024) : options.length;
    leafWebReadChunk(
        requestId,
        handle,
        static_cast<double>(options.offset),
        static_cast<double>(requestLength)
    );
}
