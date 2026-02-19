#ifndef LEAF_IOS_LEAFRENDERER_H
#define LEAF_IOS_LEAFRENDERER_H

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface LeafRenderer : NSObject

@property (nonatomic, copy, nullable) dispatch_block_t onEngineReady;

- (instancetype)initWithView:(UIView *)view NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

- (void)start;
- (void)stop;
- (void)resizeIfNeeded;

- (void)handleTouchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *_Nullable)event;
- (void)handleTouchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *_Nullable)event;
- (void)handleTouchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *_Nullable)event;
- (void)handleTouchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *_Nullable)event;

@end

NS_ASSUME_NONNULL_END

#endif // LEAF_IOS_LEAFRENDERER_H
