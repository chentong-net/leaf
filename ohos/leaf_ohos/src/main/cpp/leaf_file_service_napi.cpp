#include "leaf_file_service_napi.h"

#include "LFFileService.h"
#include "plugin/LFPlugin.h"

#include <atomic>

namespace {

enum class LeafFileRequestOp {
    Pick = 1,
    Read = 2,
    Save = 3,
};

struct FileServiceRequest {
    int op = 0;
    int requestId = 0;
    int mediaType = 0;
    int copyToSandbox = 0;
    std::string fileId;
    std::string fileName;
    std::string content;
};

std::mutex g_bridgeMutex;
napi_threadsafe_function g_requestTsfn = nullptr;
std::atomic<int> g_nextRequestId{1};

std::unordered_map<int, LFFilePickCallback> g_pickCallbacks;
std::unordered_map<int, LFFileReadCallback> g_readCallbacks;
std::unordered_map<int, LFFileSaveCallback> g_saveCallbacks;

std::string getStringArg(napi_env env, napi_value value) {
    if (!value) {
        return "";
    }

    size_t length = 0;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) {
        return "";
    }
    if (length == 0) {
        return "";
    }

    std::string result(length + 1, '\0');
    size_t actual = 0;
    if (napi_get_value_string_utf8(env, value, result.data(), result.size(), &actual) != napi_ok) {
        return "";
    }
    result.resize(actual);
    return result;
}

int32_t getIntArg(napi_env env, napi_value value) {
    int32_t out = 0;
    if (value) {
        napi_get_value_int32(env, value, &out);
    }
    return out;
}

bool getBoolArg(napi_env env, napi_value value) {
    bool out = false;
    if (value) {
        napi_get_value_bool(env, value, &out);
    }
    return out;
}

int64_t getInt64Arg(napi_env env, napi_value value) {
    int64_t out = 0;
    if (value) {
        napi_get_value_int64(env, value, &out);
    }
    return out;
}

void callJsRequest(napi_env env, napi_value jsCallback, void* context, void* data) {
    (void) context;
    auto* request = static_cast<FileServiceRequest*>(data);
    if (!request) {
        return;
    }

    if (!env || !jsCallback) {
        delete request;
        return;
    }

    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value argv[7] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    napi_create_int32(env, request->op, &argv[0]);
    napi_create_int32(env, request->requestId, &argv[1]);
    napi_create_int32(env, request->mediaType, &argv[2]);
    napi_get_boolean(env, request->copyToSandbox != 0, &argv[3]);
    napi_create_string_utf8(env, request->fileId.c_str(), NAPI_AUTO_LENGTH, &argv[4]);
    napi_create_string_utf8(env, request->fileName.c_str(), NAPI_AUTO_LENGTH, &argv[5]);
    napi_create_string_utf8(env, request->content.c_str(), NAPI_AUTO_LENGTH, &argv[6]);

    napi_call_function(env, undefined, jsCallback, 7, argv, nullptr);
    delete request;
}

bool postJsRequest(FileServiceRequest* request) {
    if (!request) {
        return false;
    }

    napi_threadsafe_function tsfn = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_bridgeMutex);
        tsfn = g_requestTsfn;
    }

    if (!tsfn) {
        return false;
    }

    napi_status status = napi_call_threadsafe_function(tsfn, request, napi_tsfn_nonblocking);
    return status == napi_ok;
}

class OhosFileService final : public LFFileService {
public:
    void pickFile(const LFFilePickOptions& options, LFFilePickCallback callback) override {
        if (!callback) {
            return;
        }

        const int requestId = g_nextRequestId.fetch_add(1);
        {
            std::lock_guard<std::mutex> lock(g_bridgeMutex);
            g_pickCallbacks[requestId] = std::move(callback);
        }

        auto* request = new FileServiceRequest();
        request->op = static_cast<int>(LeafFileRequestOp::Pick);
        request->requestId = requestId;
        request->mediaType = static_cast<int>(options.mediaType);
        request->copyToSandbox = options.copyToSandbox ? 1 : 0;

        if (!postJsRequest(request)) {
            delete request;
            LFFilePickCallback failedCb;
            {
                std::lock_guard<std::mutex> lock(g_bridgeMutex);
                auto it = g_pickCallbacks.find(requestId);
                if (it != g_pickCallbacks.end()) {
                    failedCb = std::move(it->second);
                    g_pickCallbacks.erase(it);
                }
            }
            if (failedCb) {
                LFFilePickResult result;
                result.ok = false;
                result.error = "ohos_bridge_unavailable";
                LFPluginCenter::dispatchToMain([cb = std::move(failedCb), result]() mutable {
                    cb(result);
                });
            }
        }
    }

    void readFile(const std::string& fileId, LFFileReadCallback callback) override {
        if (!callback) {
            return;
        }

        const int requestId = g_nextRequestId.fetch_add(1);
        {
            std::lock_guard<std::mutex> lock(g_bridgeMutex);
            g_readCallbacks[requestId] = std::move(callback);
        }

        auto* request = new FileServiceRequest();
        request->op = static_cast<int>(LeafFileRequestOp::Read);
        request->requestId = requestId;
        request->fileId = fileId;

        if (!postJsRequest(request)) {
            delete request;
            LFFileReadCallback failedCb;
            {
                std::lock_guard<std::mutex> lock(g_bridgeMutex);
                auto it = g_readCallbacks.find(requestId);
                if (it != g_readCallbacks.end()) {
                    failedCb = std::move(it->second);
                    g_readCallbacks.erase(it);
                }
            }
            if (failedCb) {
                LFFileReadResult result;
                result.ok = false;
                result.error = "ohos_bridge_unavailable";
                LFPluginCenter::dispatchToMain([cb = std::move(failedCb), result]() mutable {
                    cb(result);
                });
            }
        }
    }

    void saveFile(const LFFileSaveOptions& options, const std::string& content, LFFileSaveCallback callback) override {
        if (!callback) {
            return;
        }

        const int requestId = g_nextRequestId.fetch_add(1);
        {
            std::lock_guard<std::mutex> lock(g_bridgeMutex);
            g_saveCallbacks[requestId] = std::move(callback);
        }

        auto* request = new FileServiceRequest();
        request->op = static_cast<int>(LeafFileRequestOp::Save);
        request->requestId = requestId;
        request->fileName = options.fileName;
        request->content = content;

        if (!postJsRequest(request)) {
            delete request;
            LFFileSaveCallback failedCb;
            {
                std::lock_guard<std::mutex> lock(g_bridgeMutex);
                auto it = g_saveCallbacks.find(requestId);
                if (it != g_saveCallbacks.end()) {
                    failedCb = std::move(it->second);
                    g_saveCallbacks.erase(it);
                }
            }
            if (failedCb) {
                LFFileSaveResult result;
                result.ok = false;
                result.error = "ohos_bridge_unavailable";
                LFPluginCenter::dispatchToMain([cb = std::move(failedCb), result]() mutable {
                    cb(result);
                });
            }
        }
    }
};

napi_value InitFileServiceBridge(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    napi_value ret = nullptr;
    if (argc < 1) {
        napi_get_boolean(env, false, &ret);
        return ret;
    }

    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, argv[0], &type) != napi_ok || type != napi_function) {
        napi_get_boolean(env, false, &ret);
        return ret;
    }

    {
        std::lock_guard<std::mutex> lock(g_bridgeMutex);
        if (g_requestTsfn) {
            napi_release_threadsafe_function(g_requestTsfn, napi_tsfn_abort);
            g_requestTsfn = nullptr;
        }
    }

    napi_value resourceName = nullptr;
    napi_create_string_utf8(env, "leaf_file_service_ohos_request", NAPI_AUTO_LENGTH, &resourceName);

    napi_threadsafe_function tsfn = nullptr;
    napi_status status = napi_create_threadsafe_function(
            env,
            argv[0],
            nullptr,
            resourceName,
            64,
            1,
            nullptr,
            nullptr,
            nullptr,
            callJsRequest,
            &tsfn);

    if (status != napi_ok || !tsfn) {
        napi_get_boolean(env, false, &ret);
        return ret;
    }

    {
        std::lock_guard<std::mutex> lock(g_bridgeMutex);
        g_requestTsfn = tsfn;
    }

    static std::shared_ptr<LFFileService> s_fileService = std::make_shared<OhosFileService>();
    LFFileSystem::setFileService(s_fileService);

    napi_get_boolean(env, true, &ret);
    return ret;
}

napi_value NotifyPickResult(napi_env env, napi_callback_info info) {
    size_t argc = 9;
    napi_value argv[9] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (argc < 9) {
        return nullptr;
    }

    const int requestId = getIntArg(env, argv[0]);
    const bool success = getBoolArg(env, argv[1]);
    const bool canceled = getBoolArg(env, argv[2]);
    const std::string fileId = getStringArg(env, argv[3]);
    const std::string name = getStringArg(env, argv[4]);
    const std::string path = getStringArg(env, argv[5]);
    const std::string mimeType = getStringArg(env, argv[6]);
    const int64_t size = getInt64Arg(env, argv[7]);
    const std::string error = getStringArg(env, argv[8]);

    LFFilePickCallback callback;
    {
        std::lock_guard<std::mutex> lock(g_bridgeMutex);
        auto it = g_pickCallbacks.find(requestId);
        if (it != g_pickCallbacks.end()) {
            callback = std::move(it->second);
            g_pickCallbacks.erase(it);
        }
    }

    if (!callback) {
        return nullptr;
    }

    LFFilePickResult result;
    result.ok = success;
    result.canceled = canceled;
    result.error = error;
    if (success) {
        LFFileDescriptor descriptor;
        descriptor.fileId = fileId;
        descriptor.name = name;
        descriptor.path = path;
        descriptor.hasLocalPath = !path.empty();
        descriptor.mimeType = mimeType;
        descriptor.size = size;
        result.files.push_back(descriptor);
    }

    LFPluginCenter::dispatchToMain([cb = std::move(callback), result]() mutable {
        cb(result);
    });
    return nullptr;
}

napi_value NotifyReadResult(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    napi_value argv[5] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (argc < 5) {
        return nullptr;
    }

    const int requestId = getIntArg(env, argv[0]);
    const bool success = getBoolArg(env, argv[1]);
    const bool canceled = getBoolArg(env, argv[2]);
    const std::string content = getStringArg(env, argv[3]);
    const std::string error = getStringArg(env, argv[4]);

    LFFileReadCallback callback;
    {
        std::lock_guard<std::mutex> lock(g_bridgeMutex);
        auto it = g_readCallbacks.find(requestId);
        if (it != g_readCallbacks.end()) {
            callback = std::move(it->second);
            g_readCallbacks.erase(it);
        }
    }

    if (!callback) {
        return nullptr;
    }

    LFFileReadResult result;
    result.ok = success;
    result.canceled = canceled;
    result.content = content;
    result.error = error;

    LFPluginCenter::dispatchToMain([cb = std::move(callback), result]() mutable {
        cb(result);
    });
    return nullptr;
}

napi_value NotifySaveResult(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    napi_value argv[5] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (argc < 5) {
        return nullptr;
    }

    const int requestId = getIntArg(env, argv[0]);
    const bool success = getBoolArg(env, argv[1]);
    const bool canceled = getBoolArg(env, argv[2]);
    const std::string path = getStringArg(env, argv[3]);
    const std::string error = getStringArg(env, argv[4]);

    LFFileSaveCallback callback;
    {
        std::lock_guard<std::mutex> lock(g_bridgeMutex);
        auto it = g_saveCallbacks.find(requestId);
        if (it != g_saveCallbacks.end()) {
            callback = std::move(it->second);
            g_saveCallbacks.erase(it);
        }
    }

    if (!callback) {
        return nullptr;
    }

    LFFileSaveResult result;
    result.ok = success;
    result.canceled = canceled;
    result.path = path;
    result.error = error;

    LFPluginCenter::dispatchToMain([cb = std::move(callback), result]() mutable {
        cb(result);
    });
    return nullptr;
}

} // namespace

void leafRegisterFileServiceNapi(napi_env env, napi_value exports) {
    napi_property_descriptor descriptors[] = {
            {"initFileServiceBridge", nullptr, InitFileServiceBridge, nullptr, nullptr, nullptr, napi_default, nullptr},
            {"notifyPickResult", nullptr, NotifyPickResult, nullptr, nullptr, nullptr, napi_default, nullptr},
            {"notifyReadResult", nullptr, NotifyReadResult, nullptr, nullptr, nullptr, napi_default, nullptr},
            {"notifySaveResult", nullptr, NotifySaveResult, nullptr, nullptr, nullptr, napi_default, nullptr},
    };

    napi_define_properties(env, exports, 4, descriptors);
}
