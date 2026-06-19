#import "LFNativePluginBridge.h"
#import "LFPluginDispatcher.h"

#include "plugin/LFNativeSender.h"

#include <atomic>
#include <mutex>

namespace {

std::atomic<int32_t> g_bridgeRefCount{0};
std::mutex g_bridgeMutex;
bool g_bridgeBound = false;

NSString *toNSString(const std::string& text) {
    if (text.empty()) {
        return @"";
    }
    NSString *value = [[NSString alloc] initWithBytes:text.data()
                                               length:text.size()
                                             encoding:NSUTF8StringEncoding];
    return value ?: @"";
}

void dispatchMethodCallToObjC(const LFMethodCall& call) {
    const int32_t requestId = call.requestId;
    const std::string method = call.method;
    const std::string args = call.args;

    auto block = ^{
        @autoreleasepool {
            [[LFPluginDispatcher sharedInstance] dispatchWithMethod:toNSString(method)
                                                          requestId:requestId
                                                               args:toNSString(args)];
        }
    };

    if ([NSThread isMainThread]) {
        block();
    } else {
        dispatch_async(dispatch_get_main_queue(), block);
    }
}

} // namespace

@implementation LFNativePluginBridge

+ (void)install {
    const int32_t current = g_bridgeRefCount.fetch_add(1) + 1;
    if (current != 1) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_bridgeMutex);
    if (g_bridgeBound) {
        return;
    }
    LFNativeSender::getInstance().bindTarget(dispatchMethodCallToObjC);
    g_bridgeBound = true;
}

+ (void)uninstall {
    const int32_t previous = g_bridgeRefCount.fetch_sub(1);
    if (previous <= 0) {
        g_bridgeRefCount.store(0);
        return;
    }

    if (previous > 1) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_bridgeMutex);
    if (!g_bridgeBound) {
        return;
    }
    LFNativeSender::getInstance().bindTarget(nullptr);
    LFNativeSender::getInstance().clearPending();
    g_bridgeBound = false;
}

@end
