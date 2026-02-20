#ifndef LEAF_IOS_LEAFRENDERER_H
#define LEAF_IOS_LEAFRENDERER_H

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^LeafTextInputFocusChangedHandler)(BOOL focused);

@interface LeafRenderer : NSObject

@property (nonatomic, copy, nullable) dispatch_block_t onEngineReady;
@property (nonatomic, copy, nullable) LeafTextInputFocusChangedHandler onTextInputFocusChanged;

- (instancetype)initWithView:(UIView *)view NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

- (void)start;
- (void)stop;
- (void)resizeIfNeeded;

- (void)handleTouchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *_Nullable)event;
- (void)handleTouchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *_Nullable)event;
- (void)handleTouchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *_Nullable)event;
- (void)handleTouchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *_Nullable)event;

- (void)queueKeyEventWithType:(int)type keyCode:(int)keyCode modifiers:(uint32_t)modifiers repeat:(BOOL)repeat;
- (void)queueCharInput:(uint32_t)codepoint;

- (int)keyEventTypeDown;
- (int)keyEventTypeUp;
- (int)mapIOSHidKeyCode:(NSInteger)hidUsage;
- (int)mapIOSInputKey:(NSString *_Nullable)input;
- (uint32_t)mapIOSModifierFlags:(UIKeyModifierFlags)modifierFlags;

@end

NS_ASSUME_NONNULL_END

#endif // LEAF_IOS_LEAFRENDERER_H
