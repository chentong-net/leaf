//
// Created by Chen Tong on 2026/2/18.
//

#ifndef LEAF_LFNATIVESENDER_H
#define LEAF_LFNATIVESENDER_H

#include "LFMethodTypes.h"
#include <atomic>

/**
* 原生消息发送器
*/
class LFNativeSender {
public:
    static LFNativeSender& getInstance();

    void bindTarget(LFNativeTarget target);

    int32_t send(const std::string& method, const std::string& args, LFMethodResultCallback callback = nullptr);

    bool resolve(const LFMethodResult& result);

    void clearPending();

    bool isTargetBound() const;

    size_t pendingCount() const;

private:
    LFNativeSender() = default;

    std::atomic<int32_t> m_nextRequestId{1};
    mutable std::mutex m_mutex;
    LFNativeTarget m_target;
    std::unordered_map<int32_t, LFMethodResultCallback> m_pendingCallbacks;
};

#endif // LEAF_LFNATIVESENDER_H
