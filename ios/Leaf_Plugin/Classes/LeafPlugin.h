#ifndef LEAF_PLUGIN_LEAFPLUGIN_H
#define LEAF_PLUGIN_LEAFPLUGIN_H

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LeafMethodCall <NSObject>

@property (nonatomic, assign, readonly) int32_t requestId;
@property (nonatomic, copy, readonly) NSString *method;
@property (nonatomic, copy, readonly) NSString *args;

@end

@protocol LeafResult <NSObject>

- (void)success:(int32_t)requestId data:(NSString *)data;
- (void)error:(int32_t)requestId code:(int32_t)code error:(NSString *)error canceled:(BOOL)canceled;

@end

@protocol LeafPlugin <NSObject>

- (NSString *)pluginName;
- (BOOL)canHandle:(NSString *)method;
- (void)onMethodCall:(id<LeafMethodCall>)call result:(id<LeafResult>)result;

@end

NS_INLINE NSString *LFRequirePluginName(NSString * _Nullable name) {
    if (name == nil || name.length == 0) {
        @throw [NSException exceptionWithName:NSInvalidArgumentException
                                       reason:@"pluginName is empty"
                                     userInfo:nil];
    }
    return name;
}

NS_ASSUME_NONNULL_END

#endif // LEAF_PLUGIN_LEAFPLUGIN_H
