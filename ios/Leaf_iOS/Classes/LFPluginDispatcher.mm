#import "LFPluginDispatcher.h"
#import "LFPluginRegistry.h"
#if __has_include(<Leaf_Plugin/LeafPlugin.h>)
#import <Leaf_Plugin/LeafPlugin.h>
#else
#import "LeafPlugin.h"
#endif

#include "plugin/LFNativeSender.h"

typedef void (^LFDispatchSuccessEmitter)(int32_t requestId, NSString *data);
typedef void (^LFDispatchErrorEmitter)(int32_t requestId, int32_t code, NSString *error, BOOL canceled);

namespace {

static std::string toStdString(NSString *text) {
    if (text == nil || text.length == 0) {
        return "";
    }
    const char *raw = text.UTF8String;
    if (raw == nullptr) {
        return "";
    }
    return std::string(raw);
}

} // namespace

@interface LFDispatchMethodCall : NSObject <LeafMethodCall>
@property (nonatomic, assign, readonly) int32_t requestId;
@property (nonatomic, copy, readonly) NSString *method;
@property (nonatomic, copy, readonly) NSString *args;
- (instancetype)initWithRequestId:(int32_t)requestId method:(NSString *)method args:(NSString *)args;
@end

@implementation LFDispatchMethodCall

- (instancetype)initWithRequestId:(int32_t)requestId method:(NSString *)method args:(NSString *)args {
    self = [super init];
    if (self) {
        _requestId = requestId;
        _method = [method copy];
        _args = [args copy];
    }
    return self;
}

@end

@interface LFDispatchResult : NSObject <LeafResult>
- (instancetype)initWithRequestId:(int32_t)requestId
                    successEmitter:(LFDispatchSuccessEmitter)successEmitter
                      errorEmitter:(LFDispatchErrorEmitter)errorEmitter;
- (BOOL)isCompleted;
@end

@implementation LFDispatchResult {
    BOOL _completed;
    int32_t _requestId;
    LFDispatchSuccessEmitter _successEmitter;
    LFDispatchErrorEmitter _errorEmitter;
}

- (instancetype)initWithRequestId:(int32_t)requestId
                    successEmitter:(LFDispatchSuccessEmitter)successEmitter
                      errorEmitter:(LFDispatchErrorEmitter)errorEmitter {
    self = [super init];
    if (self) {
        _completed = NO;
        _requestId = requestId;
        _successEmitter = [successEmitter copy];
        _errorEmitter = [errorEmitter copy];
    }
    return self;
}

- (BOOL)markCompleted {
    @synchronized (self) {
        if (_completed) {
            return NO;
        }
        _completed = YES;
        return YES;
    }
}

- (BOOL)isCompleted {
    @synchronized (self) {
        return _completed;
    }
}

- (void)success:(int32_t)requestId data:(NSString *)data {
    if (![self markCompleted]) {
        return;
    }
    int32_t finalRequestId = requestId > 0 ? requestId : _requestId;
    if (_successEmitter) {
        _successEmitter(finalRequestId, data ?: @"");
    }
}

- (void)error:(int32_t)requestId code:(int32_t)code error:(NSString *)error canceled:(BOOL)canceled {
    if (![self markCompleted]) {
        return;
    }
    int32_t finalRequestId = requestId > 0 ? requestId : _requestId;
    if (_errorEmitter) {
        _errorEmitter(finalRequestId, code, error ?: @"", canceled);
    }
}

@end

@interface LFPluginDispatcher ()
@property (nonatomic, strong, readwrite) LFPluginRegistry *registry;
@property (nonatomic, copy, nullable) LFPluginResultEmitter resultEmitterBlock;
@end

@implementation LFPluginDispatcher

+ (instancetype)sharedInstance {
    static LFPluginDispatcher *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[LFPluginDispatcher alloc] initPrivate];
    });
    return instance;
}

- (instancetype)init {
    [NSException raise:NSInternalInconsistencyException format:@"Use +[LFPluginDispatcher sharedInstance]."];
    return nil;
}

- (instancetype)initPrivate {
    self = [super init];
    if (self) {
        _registry = [LFPluginRegistry sharedInstance];
        _resultEmitterBlock = ^(int32_t requestId, BOOL ok, int32_t code, BOOL canceled, NSString *data, NSString *error) {
            LFMethodResult result;
            result.requestId = requestId;
            result.ok = ok;
            result.code = code;
            result.canceled = canceled;
            result.data = toStdString(data);
            result.error = toStdString(error);
            LFNativeSender::getInstance().resolve(result);
        };
    }
    return self;
}

- (void)setResultEmitter:(LFPluginResultEmitter)emitter {
    self.resultEmitterBlock = [emitter copy];
}

- (void)dispatchWithMethod:(NSString *)method requestId:(int32_t)requestId args:(NSString *)args {
    if (method == nil || method.length == 0) {
        [self emitErrorWithRequestId:requestId code:-1 error:@"method_empty" canceled:NO];
        return;
    }

    id<LeafPlugin> plugin = [self.registry findByMethod:method];
    if (plugin == nil) {
        [self emitErrorWithRequestId:requestId code:-404 error:@"method_not_implemented" canceled:NO];
        return;
    }

    LFDispatchResult *result = [[LFDispatchResult alloc] initWithRequestId:requestId
                                                             successEmitter:^(int32_t callbackRequestId, NSString *data) {
        [self emitSuccessWithRequestId:callbackRequestId data:data];
    } errorEmitter:^(int32_t callbackRequestId, int32_t code, NSString *error, BOOL canceled) {
        [self emitErrorWithRequestId:callbackRequestId code:code error:error canceled:canceled];
    }];

    @try {
        LFDispatchMethodCall *call = [[LFDispatchMethodCall alloc] initWithRequestId:requestId
                                                                                method:method
                                                                                  args:(args ?: @"")];
        [plugin onMethodCall:call result:result];
    } @catch (NSException *exception) {
        if ([result isCompleted]) {
            return;
        }
        NSString *error = exception.reason.length > 0 ? exception.reason : @"plugin_dispatch_throw";
        [result error:requestId code:-500 error:error canceled:NO];
    }
}

- (void)emitSuccessWithRequestId:(int32_t)requestId data:(NSString *)data {
    LFPluginResultEmitter emitter = self.resultEmitterBlock;
    if (!emitter) {
        return;
    }
    emitter(requestId, YES, 0, NO, data ?: @"", @"");
}

- (void)emitErrorWithRequestId:(int32_t)requestId code:(int32_t)code error:(NSString *)error canceled:(BOOL)canceled {
    LFPluginResultEmitter emitter = self.resultEmitterBlock;
    if (!emitter) {
        return;
    }
    NSString *errorText = (error != nil && error.length > 0) ? error : @"unknown_error";
    emitter(requestId, NO, code, canceled, @"", errorText);
}

@end
