//
// Created by Chen Tong on 2026/2/18.
//

#include "LFNativeSender.h"

namespace {

LFMethodResult makeErrorResult(int32_t requestId, int32_t code, const char* error) {
    LFMethodResult result;
    result.requestId = requestId;
    result.ok = false;
    result.code = code;
    result.error = error ? error : "unknown_error";
    return result;
}

} // namespace

LFNativeSender& LFNativeSender::getInstance() {
    static LFNativeSender instance;
    return instance;
}

void LFNativeSender::bindTarget(LFNativeTarget target) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_target = std::move(target);
}

int32_t LFNativeSender::send(const std::string& method, const std::string& args, LFMethodResultCallback callback) {
    if (method.empty()) {
        if (callback) {
            callback(makeErrorResult(0, -1, "method_empty"));
        }
        return 0;
    }

    const int32_t requestId = m_nextRequestId.fetch_add(1);

    LFNativeTarget target;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        target = m_target;
        if (callback) {
            m_pendingCallbacks[requestId] = std::move(callback);
        }
    }

    if (!target) {
        LFMethodResultCallback pending;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_pendingCallbacks.find(requestId);
            if (it != m_pendingCallbacks.end()) {
                pending = std::move(it->second);
                m_pendingCallbacks.erase(it);
            }
        }
        if (pending) {
            pending(makeErrorResult(requestId, -2, "native_target_unbound"));
        }
        return requestId;
    }

    LFMethodCall call;
    call.requestId = requestId;
    call.method = method;
    call.args = args;

    try {
        target(call);
    } catch (...) {
        LFMethodResultCallback pending;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_pendingCallbacks.find(requestId);
            if (it != m_pendingCallbacks.end()) {
                pending = std::move(it->second);
                m_pendingCallbacks.erase(it);
            }
        }
        if (pending) {
            pending(makeErrorResult(requestId, -3, "native_target_threw"));
        }
    }
    return requestId;
}

bool LFNativeSender::resolve(const LFMethodResult& result) {
    LFMethodResultCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_pendingCallbacks.find(result.requestId);
        if (it == m_pendingCallbacks.end()) {
            return false;
        }
        callback = std::move(it->second);
        m_pendingCallbacks.erase(it);
    }

    if (callback) {
        callback(result);
        return true;
    }
    return false;
}

void LFNativeSender::clearPending() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pendingCallbacks.clear();
}

bool LFNativeSender::isTargetBound() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<bool>(m_target);
}

size_t LFNativeSender::pendingCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pendingCallbacks.size();
}
