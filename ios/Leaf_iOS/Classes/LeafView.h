#ifndef LEAF_IOS_LEAFVIEW_H
#define LEAF_IOS_LEAFVIEW_H

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface LeafView : UIView <UITextInput>

@property (nonatomic, copy, nullable) dispatch_block_t onEngineReady;

- (void)startRendering;
- (void)stopRendering;

@end

NS_ASSUME_NONNULL_END

#endif // LEAF_IOS_LEAFVIEW_H
