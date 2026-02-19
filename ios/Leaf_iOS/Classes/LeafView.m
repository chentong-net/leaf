#import "LeafView.h"
#import "LeafRenderer.h"
#import <QuartzCore/CAEAGLLayer.h>

@interface LeafView ()
@property (nonatomic, strong) LeafRenderer *renderer;
@end

@implementation LeafView

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self leaf_commonInit];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    if (self) {
        [self leaf_commonInit];
    }
    return self;
}

- (void)leaf_commonInit {
    self.contentScaleFactor = UIScreen.mainScreen.scale;
    self.multipleTouchEnabled = YES;
    self.opaque = YES;
    self.renderer = [[LeafRenderer alloc] initWithView:self];
}

- (void)setOnEngineReady:(dispatch_block_t)onEngineReady {
    _onEngineReady = [onEngineReady copy];
    self.renderer.onEngineReady = _onEngineReady;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    [self.renderer resizeIfNeeded];
}

- (void)didMoveToWindow {
    [super didMoveToWindow];
    if (self.window) {
        [self startRendering];
    } else {
        [self stopRendering];
    }
}

- (void)startRendering {
    [self.renderer start];
}

- (void)stopRendering {
    [self.renderer stop];
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.renderer handleTouchesBegan:touches withEvent:event];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.renderer handleTouchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.renderer handleTouchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.renderer handleTouchesCancelled:touches withEvent:event];
}

@end
