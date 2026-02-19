#ifndef LEAF_IOS_LFPLUGINDISPATCHER_H
#define LEAF_IOS_LFPLUGINDISPATCHER_H

#import <Foundation/Foundation.h>

@class LFPluginRegistry;

NS_ASSUME_NONNULL_BEGIN

typedef void (^LFPluginResultEmitter)(
    int32_t requestId,
    BOOL ok,
    int32_t code,
    BOOL canceled,
    NSString *data,
    NSString *error
);

@interface LFPluginDispatcher : NSObject

@property (nonatomic, strong, readonly) LFPluginRegistry *registry;

+ (instancetype)sharedInstance;

- (void)setResultEmitter:(nullable LFPluginResultEmitter)emitter;
- (void)dispatchWithMethod:(nullable NSString *)method
                 requestId:(int32_t)requestId
                      args:(nullable NSString *)args;

@end

NS_ASSUME_NONNULL_END

#endif // LEAF_IOS_LFPLUGINDISPATCHER_H
