//
// Created by Chen Tong on 2026/2/18.
//

#ifndef LEAF_LFMETHODTYPES_H
#define LEAF_LFMETHODTYPES_H

#include "LFDef.h"

/**
* MethodChannel请求结构
*/
struct LFMethodCall {
    int32_t requestId = 0;
    std::string method;
    std::string args;
};

/**
* MethodChannel结果结构
*/
struct LFMethodResult {
    int32_t requestId = 0;
    bool ok = false;
    bool canceled = false;
    int32_t code = 0;
    std::string data;
    std::string error;
};

using LFMethodResultCallback = std::function<void(const LFMethodResult&)>;
using LFNativeTarget = std::function<void(const LFMethodCall&)>;

#endif // LEAF_LFMETHODTYPES_H
