#ifndef LEAF_IOS_LFNATIVEPLUGINBRIDGE_H
#define LEAF_IOS_LFNATIVEPLUGINBRIDGE_H

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LFNativePluginBridge : NSObject

+ (void)install;
+ (void)uninstall;

@end

NS_ASSUME_NONNULL_END

#endif // LEAF_IOS_LFNATIVEPLUGINBRIDGE_H
