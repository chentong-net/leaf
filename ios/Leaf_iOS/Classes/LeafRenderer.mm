//
// Created by Chen Tong on 2026/2/7.
//

#import "LeafRenderer.h"
#import "LFNativePluginBridge.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CADisplayLink.h>

#include "LFEngine.h"
#include "LFResourceProvider.h"
#include "LFAppLaunch.h"
#include <cstring>
#include <deque>
#include <mutex>
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

static const int KEY_EVENT_DOWN = 0;
static const int KEY_EVENT_UP = 1;

static const int LF_KEY_UNKNOWN = 0;
static const int LF_KEY_ENTER = 13;
static const int LF_KEY_TAB = 9;
static const int LF_KEY_BACKSPACE = 8;
static const int LF_KEY_ESCAPE = 27;
static const int LF_KEY_DELETE = 127;
static const int LF_KEY_LEFT = 1001;
static const int LF_KEY_RIGHT = 1002;
static const int LF_KEY_UP = 1003;
static const int LF_KEY_DOWN = 1004;
static const int LF_KEY_HOME = 1005;
static const int LF_KEY_END = 1006;

static const uint32_t LF_MOD_NONE = 0;
static const uint32_t LF_MOD_SHIFT = 1 << 0;
static const uint32_t LF_MOD_CTRL = 1 << 1;
static const uint32_t LF_MOD_ALT = 1 << 2;
static const uint32_t LF_MOD_SUPER = 1 << 3;

struct NativeKeyEvent {
    int type = KEY_EVENT_DOWN;
    int keyCode = LF_KEY_UNKNOWN;
    uint32_t modifiers = LF_MOD_NONE;
    bool repeat = false;
};

} // namespace

@interface LeafRenderer ()
@property (nonatomic, weak) UIView* view;
@property (nonatomic, strong) CAMetalLayer* metalLayer;
@property (nonatomic, strong) CADisplayLink* displayLink;
- (void)initializeEngineIfNeeded;
- (void)drawFrame;
- (void)processKeyInputEvents;
- (void)syncTextInputFocusState;
- (void)dispatchTouches:(NSSet<UITouch *> *)touches withEvent:(UIEvent *_Nullable)event type:(LFTouchEventType)type;
- (int32_t)touchIdForTouch:(UITouch *)touch createIfMissing:(BOOL)createIfMissing;
- (void)removeTouchIdForTouch:(UITouch *)touch;
@end

@implementation LeafRenderer {
    NVGcontext* _vg;
    BOOL _engineInitialized;
    CFTimeInterval _lastFrameTimestamp;

    std::unordered_map<uintptr_t, int32_t> _touchIdMap;
    int32_t _nextTouchId;
    std::mutex _inputQueueMutex;
    std::deque<NativeKeyEvent> _keyEventQueue;
    std::deque<uint32_t> _charInputQueue;
    BOOL _lastTextInputFocused;
}

- (instancetype)initWithView:(UIView *)view {
    self = [super init];
    if (!self) return nil;

    _view = view;
    _vg = nullptr;
    _engineInitialized = NO;
    _lastFrameTimestamp = 0;
    _nextTouchId = 1;
    _lastTextInputFocused = NO;

    _metalLayer = (CAMetalLayer *)view.layer;
    _metalLayer.device = MTLCreateSystemDefaultDevice();
    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalLayer.opaque = YES;

    [self initializeEngineIfNeeded];
    [self resizeIfNeeded];

    return self;
}

- (void)dealloc {
    [self stop];

    if (_vg) {
        nvgDeleteMTL(_vg);
        _vg = nullptr;
    }

    [LFNativePluginBridge uninstall];
}

- (void)initializeEngineIfNeeded {
    if (_engineInitialized) return;

    int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES;
    _vg = nvgCreateMTL((__bridge void*)_metalLayer, flags);
    if (!_vg) {
        LF_LOGI("LeafRenderer: failed to create NanoVG Metal context");
        return;
    }

    LFEngine::getInstance().init(_vg);
    LFEngine::getInstance().setBeginFrameCallback([](NVGcontext* vg) {
        mnvgClearWithColor(vg, nvgRGBA(0, 0, 0, 0));
    });

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
    if (!_engineInitialized || CGRectIsEmpty(self.view.bounds)) return;

    CGFloat scale = self.view.window.screen.scale ?: UIScreen.mainScreen.scale;
    CGFloat logicalWidth = self.view.bounds.size.width;
    CGFloat logicalHeight = self.view.bounds.size.height;

    CGSize drawableSize = CGSizeMake(logicalWidth * scale, logicalHeight * scale);
    _metalLayer.drawableSize = drawableSize;

    LFEngine::getInstance().setWindowSize(logicalWidth, logicalHeight, scale);
}

- (void)drawFrame {
    if (!_engineInitialized || !_vg) return;

    CFTimeInterval now = CACurrentMediaTime();
    float dt = _lastFrameTimestamp > 0 ? static_cast<float>(now - _lastFrameTimestamp) : (1.0f / 60.0f);
    _lastFrameTimestamp = now;

    [self processKeyInputEvents];
    LFEngine::getInstance().update(dt);
    LFEngine::getInstance().render();
    [self syncTextInputFocusState];
}

- (void)processKeyInputEvents {
    std::deque<NativeKeyEvent> keyEvents;
    std::deque<uint32_t> charInputs;
    {
        std::lock_guard<std::mutex> lock(_inputQueueMutex);
        keyEvents.swap(_keyEventQueue);
        charInputs.swap(_charInputQueue);
    }

    for (const NativeKeyEvent& event : keyEvents) {
        LFKeyEventType type = (event.type == KEY_EVENT_UP) ? LFKeyEventType::Up : LFKeyEventType::Down;
        LFEventDispatcher::getInstance().dispatchKeyEvent(
            type,
            static_cast<LFKeyCode>(event.keyCode),
            event.modifiers,
            event.repeat
        );
    }

    for (uint32_t codepoint : charInputs) {
        if (codepoint > 0) {
            LFEventDispatcher::getInstance().dispatchCharInput(codepoint);
        }
    }
}

- (void)syncTextInputFocusState {
    BOOL focused = (LFEventDispatcher::getInstance().getFocusedNode() != nullptr) ? YES : NO;
    if (focused == _lastTextInputFocused) {
        return;
    }
    _lastTextInputFocused = focused;

    LeafTextInputFocusChangedHandler callback = self.onTextInputFocusChanged;
    if (!callback) {
        return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        callback(focused);
    });
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

- (void)queueKeyEventWithType:(int)type keyCode:(int)keyCode modifiers:(uint32_t)modifiers repeat:(BOOL)repeat {
    NativeKeyEvent event;
    event.type = type;
    event.keyCode = keyCode;
    event.modifiers = modifiers;
    event.repeat = repeat;
    std::lock_guard<std::mutex> lock(_inputQueueMutex);
    _keyEventQueue.push_back(event);
}

- (void)queueCharInput:(uint32_t)codepoint {
    std::lock_guard<std::mutex> lock(_inputQueueMutex);
    _charInputQueue.push_back(codepoint);
}

- (int)keyEventTypeDown {
    return KEY_EVENT_DOWN;
}

- (int)keyEventTypeUp {
    return KEY_EVENT_UP;
}

- (int)mapIOSHidKeyCode:(NSInteger)hidUsage {
    switch (hidUsage) {
        case 0x28: return LF_KEY_ENTER;
        case 0x2B: return LF_KEY_TAB;
        case 0x2A: return LF_KEY_BACKSPACE;
        case 0x29: return LF_KEY_ESCAPE;
        case 0x4C: return LF_KEY_DELETE;
        case 0x50: return LF_KEY_LEFT;
        case 0x4F: return LF_KEY_RIGHT;
        case 0x52: return LF_KEY_UP;
        case 0x51: return LF_KEY_DOWN;
        case 0x4A: return LF_KEY_HOME;
        case 0x4D: return LF_KEY_END;
        default: return LF_KEY_UNKNOWN;
    }
}

- (int)mapIOSInputKey:(NSString *_Nullable)input {
    if (!input || input.length == 0) return LF_KEY_UNKNOWN;
    unichar ch = [input characterAtIndex:0];
    switch (ch) {
        case '\n':
        case '\r': return LF_KEY_ENTER;
        case '\t': return LF_KEY_TAB;
        case 0x08: return LF_KEY_BACKSPACE;
        case 0x1B: return LF_KEY_ESCAPE;
        case 0x7F: return LF_KEY_DELETE;
        default: return LF_KEY_UNKNOWN;
    }
}

- (uint32_t)mapIOSModifierFlags:(UIKeyModifierFlags)modifierFlags {
    uint32_t mods = LF_MOD_NONE;
    if (modifierFlags & UIKeyModifierShift) mods |= LF_MOD_SHIFT;
    if (modifierFlags & UIKeyModifierControl) mods |= LF_MOD_CTRL;
    if (modifierFlags & UIKeyModifierAlternate) mods |= LF_MOD_ALT;
    if (modifierFlags & UIKeyModifierCommand) mods |= LF_MOD_SUPER;
    return mods;
}

@end
