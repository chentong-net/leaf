#define NANOVG_GLES3_IMPLEMENTATION

#import "LeafRenderer.h"
#import "LFNativePluginBridge.h"

#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES3/gl.h>
#import <QuartzCore/CAEAGLLayer.h>
#import <QuartzCore/CADisplayLink.h>

#include "LFEngine.h"
#include "LFResourceProvider.h"
#include "LFAppLaunch.h"
#include <cstring>
#include <unordered_map>

namespace {

static NSString* leafNormalizeAssetPath(NSString* rawPath) {
    if (!rawPath || rawPath.length == 0) return @"";
    if ([rawPath hasPrefix:@"./"]) {
        return [rawPath substringFromIndex:2];
    }
    return rawPath;
}

static NSString* leafResolveAssetPath(NSString* rawPath) {
    NSString* relativePath = leafNormalizeAssetPath(rawPath);
    if (!relativePath || relativePath.length == 0) return nil;

    NSString* mainPath = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:relativePath];
    if ([[NSFileManager defaultManager] fileExistsAtPath:mainPath]) {
        return mainPath;
    }

    NSBundle* classBundle = [NSBundle bundleForClass:[LeafRenderer class]];
    NSURL* assetsBundleURL = [classBundle URLForResource:@"Leaf_iOS_Assets" withExtension:@"bundle"];
    if (assetsBundleURL) {
        NSBundle* assetsBundle = [NSBundle bundleWithURL:assetsBundleURL];
        NSString* podPath = [assetsBundle.resourcePath stringByAppendingPathComponent:relativePath];
        if ([[NSFileManager defaultManager] fileExistsAtPath:podPath]) {
            return podPath;
        }
    }

    return nil;
}

static std::shared_ptr<LFData> leafLoadDataFromFile(NSString* filePath) {
    if (!filePath || filePath.length == 0) return nullptr;

    NSData* data = [NSData dataWithContentsOfFile:filePath];
    if (!data || data.length == 0) return nullptr;

    auto out = std::make_shared<LFData>();
    out->size = static_cast<size_t>(data.length);
    out->data = static_cast<unsigned char*>(malloc(out->size));
    if (!out->data) return nullptr;

    memcpy(out->data, data.bytes, out->size);
    return out;
}

} // namespace

@interface LeafRenderer ()
@property (nonatomic, weak) UIView* view;
- (void)setupOpenGLESContext;
- (void)setupRenderBuffers;
- (void)destroyRenderBuffers;
- (void)initializeEngineIfNeeded;
- (void)drawFrame;
- (void)dispatchTouches:(NSSet<UITouch *> *)touches withEvent:(UIEvent *_Nullable)event type:(LFTouchEventType)type;
- (int32_t)touchIdForTouch:(UITouch *)touch createIfMissing:(BOOL)createIfMissing;
- (void)removeTouchIdForTouch:(UITouch *)touch;
@end

@implementation LeafRenderer {
    EAGLContext* _glContext;
    CADisplayLink* _displayLink;

    GLuint _frameBuffer;
    GLuint _colorBuffer;
    GLuint _depthStencilBuffer;

    GLint _backingWidth;
    GLint _backingHeight;

    NVGcontext* _vg;
    BOOL _engineInitialized;
    CFTimeInterval _lastFrameTimestamp;

    std::unordered_map<uintptr_t, int32_t> _touchIdMap;
    int32_t _nextTouchId;
}

- (instancetype)initWithView:(UIView *)view {
    self = [super init];
    if (!self) return nil;

    _view = view;
    _frameBuffer = 0;
    _colorBuffer = 0;
    _depthStencilBuffer = 0;
    _backingWidth = 0;
    _backingHeight = 0;
    _vg = nullptr;
    _engineInitialized = NO;
    _lastFrameTimestamp = 0;
    _nextTouchId = 1;

    [self setupOpenGLESContext];
    [self setupRenderBuffers];
    [self initializeEngineIfNeeded];
    [self resizeIfNeeded];

    return self;
}

- (void)dealloc {
    [self stop];
    if (_glContext) {
        [EAGLContext setCurrentContext:_glContext];
    }

    if (_vg) {
        nvgDeleteGLES3(_vg);
        _vg = nullptr;
    }

    [self destroyRenderBuffers];
    if ([EAGLContext currentContext] == _glContext) {
        [EAGLContext setCurrentContext:nil];
    }

    [LFNativePluginBridge uninstall];
}

- (void)setupOpenGLESContext {
    CAEAGLLayer* glLayer = (CAEAGLLayer*)self.view.layer;
    glLayer.opaque = YES;
    glLayer.drawableProperties = @{
        kEAGLDrawablePropertyRetainedBacking : @NO,
        kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8
    };

    _glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (!_glContext) {
        LF_LOGI("LeafRenderer: failed to create OpenGLES3 context");
        return;
    }
    [EAGLContext setCurrentContext:_glContext];
}

- (void)setupRenderBuffers {
    if (!_glContext) return;
    [EAGLContext setCurrentContext:_glContext];

    [self destroyRenderBuffers];

    glGenFramebuffers(1, &_frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);

    glGenRenderbuffers(1, &_colorBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _colorBuffer);
    [_glContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)self.view.layer];
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorBuffer);

    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &_backingWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &_backingHeight);

    glGenRenderbuffers(1, &_depthStencilBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _depthStencilBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _backingWidth, _backingHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthStencilBuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _depthStencilBuffer);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LF_LOGI("LeafRenderer: incomplete framebuffer status=%u", status);
    }

    glBindRenderbuffer(GL_RENDERBUFFER, _colorBuffer);
}

- (void)destroyRenderBuffers {
    if (_depthStencilBuffer) {
        glDeleteRenderbuffers(1, &_depthStencilBuffer);
        _depthStencilBuffer = 0;
    }
    if (_colorBuffer) {
        glDeleteRenderbuffers(1, &_colorBuffer);
        _colorBuffer = 0;
    }
    if (_frameBuffer) {
        glDeleteFramebuffers(1, &_frameBuffer);
        _frameBuffer = 0;
    }
}

- (void)initializeEngineIfNeeded {
    if (_engineInitialized || !_glContext) return;
    [EAGLContext setCurrentContext:_glContext];

    int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES;
    _vg = nvgCreateGLES3(flags);
    if (!_vg) {
        LF_LOGI("LeafRenderer: failed to create NanoVG GLES3 context");
        return;
    }

    LFEngine::getInstance().init(_vg);
    [LFNativePluginBridge install];
    LFResourceProvider::getInstance().setAssetLoader(
        [](const std::string& path, std::function<void(std::shared_ptr<LFData>)> callback) {
            @autoreleasepool {
                NSString* rawPath = [NSString stringWithUTF8String:path.c_str()];
                NSString* resolvedPath = leafResolveAssetPath(rawPath);
                callback(leafLoadDataFromFile(resolvedPath));
            }
        }
    );
    LFEngine::getInstance().setRoot(createAppRoot());

    NSString* fontPath = leafResolveAssetPath(@"fonts/Alibaba-PuHuiTi-Regular.ttf");
    if (fontPath.length > 0) {
        nvgCreateFont(_vg, "sans", fontPath.UTF8String);
    }

    _engineInitialized = YES;
    if (self.onEngineReady) {
        self.onEngineReady();
    }
}

- (void)start {
    if (_displayLink || !_engineInitialized) return;

    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawFrame)];
    if (@available(iOS 15.0, *)) {
        _displayLink.preferredFrameRateRange = CAFrameRateRangeMake(30, 60, 60);
    } else {
        _displayLink.preferredFramesPerSecond = 60;
    }
    [_displayLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
}

- (void)stop {
    [_displayLink invalidate];
    _displayLink = nil;
    _lastFrameTimestamp = 0;
}

- (void)resizeIfNeeded {
    if (!_glContext || CGRectIsEmpty(self.view.bounds)) return;
    [EAGLContext setCurrentContext:_glContext];

    [self setupRenderBuffers];
    CGFloat scale = self.view.window.screen.scale ?: UIScreen.mainScreen.scale;
    CGFloat logicalWidth = self.view.bounds.size.width;
    CGFloat logicalHeight = self.view.bounds.size.height;
    LFEngine::getInstance().setWindowSize(logicalWidth, logicalHeight, scale);
}

- (void)drawFrame {
    if (!_engineInitialized || !_glContext || _frameBuffer == 0) return;

    [EAGLContext setCurrentContext:_glContext];

    glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
    glViewport(0, 0, _backingWidth, _backingHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    CFTimeInterval now = CACurrentMediaTime();
    float dt = _lastFrameTimestamp > 0 ? static_cast<float>(now - _lastFrameTimestamp) : (1.0f / 60.0f);
    _lastFrameTimestamp = now;

    LFEngine::getInstance().update(dt);
    LFEngine::getInstance().render();

    glBindRenderbuffer(GL_RENDERBUFFER, _colorBuffer);
    [_glContext presentRenderbuffer:GL_RENDERBUFFER];
}

- (void)handleTouchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self dispatchTouches:touches withEvent:event type:LFTouchEventType::Down];
}

- (void)handleTouchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self dispatchTouches:touches withEvent:event type:LFTouchEventType::Move];
}

- (void)handleTouchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self dispatchTouches:touches withEvent:event type:LFTouchEventType::Up];
}

- (void)handleTouchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self dispatchTouches:touches withEvent:event type:LFTouchEventType::Cancel];
}

- (void)dispatchTouches:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event type:(LFTouchEventType)type {
    auto root = LFEngine::getInstance().getRoot();
    if (!root) return;

    std::vector<LFTouchPoint> allTouches;
    std::vector<LFTouchID> changedIds;

    NSSet<UITouch*>* source = event.allTouches ?: touches;
    double timestamp = LFEngine::getInstance().getElapsedTime();

    for (UITouch* touch in source) {
        CGPoint point = [touch locationInView:self.view];
        LFTouchPoint out;
        out.id = [self touchIdForTouch:touch createIfMissing:YES];
        out.x = point.x;
        out.y = point.y;
        out.pressure = touch.maximumPossibleForce > 0 ? static_cast<float>(touch.force / touch.maximumPossibleForce) : 1.0f;
        out.timestamp = timestamp;
        allTouches.push_back(out);
    }

    for (UITouch* changedTouch in touches) {
        LFTouchID changed = [self touchIdForTouch:changedTouch createIfMissing:YES];
        changedIds.push_back(changed);
    }

    LFEventDispatcher::getInstance().dispatchTouchEvent(type, allTouches, changedIds, root);

    if (type == LFTouchEventType::Up || type == LFTouchEventType::Cancel) {
        for (UITouch* changedTouch in touches) {
            [self removeTouchIdForTouch:changedTouch];
        }
    }
}

- (int32_t)touchIdForTouch:(UITouch *)touch createIfMissing:(BOOL)createIfMissing {
    uintptr_t key = reinterpret_cast<uintptr_t>((__bridge void*)touch);
    auto it = _touchIdMap.find(key);
    if (it != _touchIdMap.end()) {
        return it->second;
    }
    if (!createIfMissing) {
        return 0;
    }

    int32_t next = _nextTouchId++;
    if (next <= 0) {
        _touchIdMap.clear();
        _nextTouchId = 1;
        next = _nextTouchId++;
    }
    _touchIdMap[key] = next;
    return next;
}

- (void)removeTouchIdForTouch:(UITouch *)touch {
    uintptr_t key = reinterpret_cast<uintptr_t>((__bridge void*)touch);
    _touchIdMap.erase(key);
}

@end
