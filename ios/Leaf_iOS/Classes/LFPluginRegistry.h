#ifndef LEAF_IOS_LFPLUGINREGISTRY_H
#define LEAF_IOS_LFPLUGINREGISTRY_H

#import <Foundation/Foundation.h>

@protocol LeafPlugin;

NS_ASSUME_NONNULL_BEGIN

@interface LFPluginRegistry : NSObject

+ (instancetype)sharedInstance;

- (void)registerPlugin:(id<LeafPlugin>)plugin;
- (void)unregisterPlugin:(nullable NSString *)pluginName;
- (void)clear;
- (nullable id<LeafPlugin>)findByMethod:(nullable NSString *)method;
- (NSArray<NSString *> *)pluginNames;

@end

NS_ASSUME_NONNULL_END

#endif // LEAF_IOS_LFPLUGINREGISTRY_H
