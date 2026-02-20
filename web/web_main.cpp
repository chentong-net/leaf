//
// Created by Chen Tong on 2026/1/22.
//

#define NANOVG_GLES3_IMPLEMENTATION

#include "LFEngine.h"
#include <emscripten.h>
#include <emscripten/em_js.h>
#include <emscripten/html5.h>
#include <emscripten/fetch.h>
#include "LFAppLaunch.h"

static const char* CANVAS_ID = "#canvas";
double dpr = 1.0f;
bool g_lastTextInputFocused = false;

namespace {

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

EM_JS(void, leaf_web_install_text_input_bridge, (), {
    if (Module.__leafTextInputBridge) {
        return;
    }

    const canvas = Module.canvas || document.getElementById('canvas');
    if (!canvas) {
        return;
    }

    const input = document.createElement('textarea');
    input.id = 'leaf-ime-input';
    input.setAttribute('autocomplete', 'off');
    input.setAttribute('autocorrect', 'off');
    input.setAttribute('autocapitalize', 'off');
    input.setAttribute('spellcheck', 'false');
    input.wrap = 'off';
    input.rows = 1;
    input.cols = 1;
    input.style.position = 'fixed';
    input.style.left = '0px';
    input.style.top = '0px';
    input.style.width = '2px';
    input.style.height = '20px';
    input.style.opacity = '0';
    input.style.pointerEvents = 'none';
    input.style.zIndex = '-1';
    input.style.border = '0';
    input.style.padding = '0';
    input.style.margin = '0';
    input.style.background = 'transparent';
    input.style.color = 'transparent';
    input.style.caretColor = 'transparent';
    input.style.outline = 'none';
    document.body.appendChild(input);

    const state = {
        input,
        focused: false,
        composing: false,
        compositionCommitted: "",
        hasCursor: false,
        cursorX: 0,
        cursorY: 0,
        cursorHeight: 16,
        lastPointerClientX: 0,
        lastPointerClientY: 0
    };

    const applyInputRect = (clientX, clientY, lineHeight) => {
        const height = Math.max(16, Number(lineHeight || 16));
        input.style.left = `${clientX}px`;
        input.style.top = `${clientY}px`;
        input.style.height = `${height}px`;
        input.style.fontSize = `${height}px`;
    };

    const toLFMods = (event) => {
        let mods = 0;
        if (event.shiftKey) mods |= (1 << 0);
        if (event.ctrlKey) mods |= (1 << 1);
        if (event.altKey) mods |= (1 << 2);
        if (event.metaKey) mods |= (1 << 3);
        return mods;
    };

    const mapLFKeyCode = (key) => {
        switch (key) {
            case 'Enter':
                return 13;
            case 'Tab':
                return 9;
            case 'Backspace':
                return 8;
            case 'Escape':
                return 27;
            case 'Delete':
                return 127;
            case 'ArrowLeft':
                return 1001;
            case 'ArrowRight':
                return 1002;
            case 'ArrowUp':
                return 1003;
            case 'ArrowDown':
                return 1004;
            case 'Home':
                return 1005;
            case 'End':
                return 1006;
            default:
                return 0;
        }
    };

    const emitKey = (isDown, event) => {
        const keyCode = mapLFKeyCode(event.key || "");
        if (!keyCode) {
            return false;
        }
        // During IME composition, editing/navigation keys should be handled by IME only.
        const isComposing = !!state.composing || !!event.isComposing;
        if (isComposing) {
            return false;
        }
        const repeat = isDown && !!event.repeat ? 1 : 0;
        Module._leafWebOnKeyEvent(isDown ? 0 : 1, keyCode, toLFMods(event), repeat);
        return true;
    };

    const emitChars = (text) => {
        if (!text) {
            return;
        }
        for (const ch of text) {
            const cp = ch.codePointAt(0);
            if (cp && cp > 0) {
                Module._leafWebOnCharInput(cp);
            }
        }
    };

    const ensureFocus = () => {
        if (!state.focused) {
            return;
        }
        if (state.hasCursor) {
            applyInputRect(state.cursorX, state.cursorY, state.cursorHeight);
        } else {
            applyInputRect(state.lastPointerClientX, state.lastPointerClientY, 16);
        }
        if (document.activeElement === input) {
            return;
        }
        try {
            input.focus({ preventScroll: true });
        } catch (e) {
            input.focus();
        }
    };

    const resetInputBuffer = () => {
        input.value = "";
        if (input.setSelectionRange) {
            input.setSelectionRange(0, 0);
        }
    };

    input.addEventListener('keydown', (event) => {
        if (!state.focused) {
            return;
        }
        if (emitKey(true, event)) {
            event.preventDefault();
        }
    });

    input.addEventListener('keyup', (event) => {
        if (!state.focused) {
            return;
        }
        if (emitKey(false, event)) {
            event.preventDefault();
        }
    });

    input.addEventListener('compositionstart', () => {
        state.composing = true;
        state.compositionCommitted = "";
    });

    input.addEventListener('compositionend', (event) => {
        state.composing = false;
        const text = (typeof event.data === 'string') ? event.data : "";
        if (text.length > 0) {
            emitChars(text);
            state.compositionCommitted = text;
            resetInputBuffer();
        }
    });

    input.addEventListener('input', (event) => {
        if (!state.focused) {
            return;
        }

        const inputType = event.inputType || "";
        if (inputType === 'insertLineBreak' || inputType === 'insertParagraph') {
            Module._leafWebOnKeyEvent(0, 13, 0, 0);
            Module._leafWebOnKeyEvent(1, 13, 0, 0);
            resetInputBuffer();
            return;
        }

        if (state.composing) {
            return;
        }

        const text = (typeof event.data === 'string' && event.data.length > 0)
            ? event.data
            : input.value;
        if (text && text.length > 0) {
            if (state.compositionCommitted.length > 0 && text === state.compositionCommitted) {
                state.compositionCommitted = "";
                resetInputBuffer();
                return;
            }
            emitChars(text);
        }
        state.compositionCommitted = "";
        resetInputBuffer();
    });

    input.addEventListener('blur', () => {
        if (!state.focused) {
            return;
        }
        setTimeout(() => {
            ensureFocus();
        }, 0);
    });

    canvas.addEventListener('mousedown', (event) => {
        state.lastPointerClientX = event.clientX || 0;
        state.lastPointerClientY = event.clientY || 0;
        ensureFocus();
    }, true);
    canvas.addEventListener('touchstart', (event) => {
        if (event.touches && event.touches.length > 0) {
            state.lastPointerClientX = event.touches[0].clientX || 0;
            state.lastPointerClientY = event.touches[0].clientY || 0;
        }
        ensureFocus();
    }, true);

    Module.__leafTextInputBridge = {
        state,
        ensureFocus,
        resetInputBuffer
    };
});

EM_JS(void, leaf_web_sync_text_input_focus, (int focused), {
    const bridge = Module.__leafTextInputBridge;
    if (!bridge) return;

    const state = bridge.state;
    state.focused = !!focused;
    if (state.focused) {
        bridge.resetInputBuffer();
        bridge.ensureFocus();
    } else {
        state.composing = false;
        bridge.resetInputBuffer();
        if (document.activeElement === state.input) {
            state.input.blur();
        }
    }
});

EM_JS(void, leaf_web_set_text_input_cursor, (float x, float y, float lineHeight), {
    const bridge = Module.__leafTextInputBridge;
    if (!bridge) return;

    const canvas = Module.canvas || document.getElementById('canvas');
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const input = bridge.state.input;
    const cssX = rect.left + x;
    const cssY = rect.top + y;
    const height = Math.max(16, lineHeight || 16);
    bridge.state.hasCursor = true;
    bridge.state.cursorX = cssX;
    bridge.state.cursorY = cssY;
    bridge.state.cursorHeight = height;

    input.style.left = `${cssX}px`;
    input.style.top = `${cssY}px`;
    input.style.height = `${height}px`;
    input.style.fontSize = `${height}px`;
});

void sync_text_input_focus() {
    const bool focused = (LFEventDispatcher::getInstance().getFocusedNode() != nullptr);
    if (focused == g_lastTextInputFocused) {
        return;
    }
    g_lastTextInputFocused = focused;
    leaf_web_sync_text_input_focus(focused ? 1 : 0);
}

} // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE
void leafWebOnKeyEvent(int type, int keyCode, int modifiers, int repeat) {
    if (keyCode == LF_KEY_UNKNOWN) {
        return;
    }
    LFKeyEventType keyType = (type == KEY_EVENT_UP) ? LFKeyEventType::Up : LFKeyEventType::Down;
    LFEventDispatcher::getInstance().dispatchKeyEvent(
        keyType,
        static_cast<LFKeyCode>(keyCode),
        static_cast<uint32_t>(modifiers),
        repeat != 0
    );
}

EMSCRIPTEN_KEEPALIVE
void leafWebOnCharInput(int codepoint) {
    if (codepoint <= 0) {
        return;
    }
    LFEventDispatcher::getInstance().dispatchCharInput(static_cast<uint32_t>(codepoint));
}

} // extern "C"

// 获取当前时间戳
double get_engine_time() {
    return LFEngine::getInstance().getElapsedTime();
}

// 统一的事件分发入口
void dispatch_to_engine(LFTouchEventType type, const std::vector<LFTouchPoint>& touches, const std::vector<LFTouchID>& changedIds) {
    auto root = LFEngine::getInstance().getRoot();
    if (root) {
        LFEventDispatcher::getInstance().dispatchTouchEvent(type, touches, changedIds, root);
    }
}

// ==========================================
// 鼠标事件适配 (模拟单指触摸)
// ==========================================
EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *userData) {
    // 只处理左键 (button 0)
    if (e->button != 0) return EM_FALSE;

    LFTouchEventType type;
    if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN) type = LFTouchEventType::Down;
    else if (eventType == EMSCRIPTEN_EVENT_MOUSEMOVE) {
        // 只有按住左键移动时才算 Drag/Move
        if (e->buttons & 1) type = LFTouchEventType::Move;
        else return EM_FALSE; // 只是 hover，移动端引擎通常忽略
    }
    else if (eventType == EMSCRIPTEN_EVENT_MOUSEUP) type = LFTouchEventType::Up;
    else return EM_FALSE;

    // 构造 TouchPoint
    LFTouchPoint p;
    p.id = 0; // 鼠标固定 ID 为 0
    p.x = e->targetX;
    p.y = e->targetY;
    p.pressure = 1.0f;
    p.timestamp = get_engine_time();

    std::vector<LFTouchPoint> touches = { p };
    std::vector<LFTouchID> changed = { 0 };

    dispatch_to_engine(type, touches, changed);

    return EM_TRUE; // 消费事件
}

// ==========================================
// 触摸事件适配 (原生多点触控)
// ==========================================
EM_BOOL touch_callback(int eventType, const EmscriptenTouchEvent *e, void *userData) {
    LFTouchEventType type;
    if (eventType == EMSCRIPTEN_EVENT_TOUCHSTART) type = LFTouchEventType::Down;
    else if (eventType == EMSCRIPTEN_EVENT_TOUCHMOVE) type = LFTouchEventType::Move;
    else if (eventType == EMSCRIPTEN_EVENT_TOUCHEND) type = LFTouchEventType::Up;
    else if (eventType == EMSCRIPTEN_EVENT_TOUCHCANCEL) type = LFTouchEventType::Cancel;
    else return EM_FALSE;

    std::vector<LFTouchPoint> allTouches;
    std::vector<LFTouchID> changedIds;

    double now = get_engine_time();

    // 1. 收集当前屏幕上所有的点 (Active Touches)
    for (int i = 0; i < e->numTouches; ++i) {
        if (!e->touches[i].isChanged) {
            // 这是一个在屏幕上但状态没变的点，也需要传给引擎保持状态
            LFTouchPoint p;
            p.id = (int)e->touches[i].identifier;
            p.x = e->touches[i].targetX;
            p.y = e->touches[i].targetY;
            p.pressure = 1.0f;
            p.timestamp = now;
            allTouches.push_back(p);
        }
    }

    // 2. 收集发生变化的点 (Changed Touches)
    // Emscripten 的结构略有不同，changedTouches 是单独的列表吗？
    // 注意：Emscripten 这里的结构包含 touches (所有点) 和 changedTouches (变化点)
    // 但 C API 中 `EmscriptenTouchEvent` 结构体直接给出了 `touches` 数组，
    // 我们需要通过 `e->touches[i].isChanged` 标志来判断。

    for (int i = 0; i < e->numTouches; ++i) {
        if (e->touches[i].isChanged) {
            LFTouchPoint p;
            p.id = (int)e->touches[i].identifier;
            p.x = e->touches[i].targetX;
            p.y = e->touches[i].targetY;
            p.pressure = 1.0f;
            p.timestamp = now;

            allTouches.push_back(p);
            changedIds.push_back(p.id);
        }
    }

    dispatch_to_engine(type, allTouches, changedIds);

    // 返回 EM_TRUE 非常重要！这会调用 e.preventDefault()
    // 防止浏览器处理滑动（比如页面滚动、下拉刷新等）
    return EM_TRUE;
}

void update_canvas_size() {
    double cssW, cssH;
    // 1. 获取 HTML 元素目前的 CSS 显示尺寸 (比如 800x600)
    emscripten_get_element_css_size(CANVAS_ID, &cssW, &cssH);

    // 2. 获取设备像素比 (Retina 屏通常是 2.0)
    dpr = emscripten_get_device_pixel_ratio();

    // 3. 计算物理像素尺寸 (比如 1600x1200)
    // 必须取整，否则可能导致渲染裂缝
    int phyW = (int)std::ceil(cssW * dpr);
    int phyH = (int)std::ceil(cssH * dpr);

    // 4. 设置 Canvas 的“绘图缓冲区”尺寸
    // 这对应 HTML 标签的 <canvas width="..." height="...">
    emscripten_set_canvas_element_size(CANVAS_ID, phyW, phyH);

    // 5. 告诉你的引擎窗口变了
    // 注意：如果是 HiDPI，你可能需要根据你的 LFEngine 逻辑决定是传 cssW 还是 phyW
    // 如果你希望 100px 在 Web 上等于 100 CSS 像素（推荐），就传 cssW
    // 并在 Render 时应用 nvgScale(vg, dpr, dpr)
    // TODO: 这里为了简单，我们先传 CSS 尺寸，让 NanoVG 自动处理
    LFEngine::getInstance().setWindowSize((float)cssW, (float)cssH, (float)dpr);

    // 如果需要手动处理 DPR，可以在这里打印日志调试
    // printf("Resize: CSS=%.0fx%.0f, Phy=%dx%d, DPR=%.2f\n", cssW, cssH, phyW, phyH, dpr);
}

EM_BOOL on_resize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData) {
    update_canvas_size();
    return EM_TRUE;
}

void loop_callback() {
    LFEngine::getInstance().update(0.016f);
    LFEngine::getInstance().render();
    sync_text_input_focus();
}

int main() {
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = 0;
    attr.depth = 0;              // NanoVG 不需要深度缓冲
    attr.stencil = 1;            // <!!! 关键修复 !!!> NanoVG 必须要有 Stencil 缓冲
    attr.antialias = 1;          // 开启抗锯齿，文字更清晰
    attr.majorVersion = 2; // WebGL 2 => GLES 3
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(ctx);

    int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES;
    NVGcontext *vg = nvgCreateGLES3(flags);

    if (!vg) return 0;

    LFEngine::getInstance().init(vg);
    leaf_web_install_text_input_bridge();
    LFEngine::getInstance().setTextInputCursorCallback([](float x, float y, float lineHeight) {
        leaf_web_set_text_input_cursor(x, y, lineHeight);
    });
    leaf_web_sync_text_input_focus(0);

    nvgCreateFont(vg, "sans", "fonts/Alibaba-PuHuiTi-Regular.ttf");

    LFResourceProvider::getInstance().setAssetLoader(
            [](const std::string &path,
               std::function<void(std::shared_ptr<LFData>)> callback) {

                emscripten_fetch_attr_t attr;
                emscripten_fetch_attr_init(&attr);

                strcpy(attr.requestMethod, "GET");
                attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY; // 直接加载到内存

                // 关键点：我们需要把 callback 传给成功/失败的静态函数
                // 我们在堆上创建一个 callback 的拷贝，生命周期交给 Fetch 管理
                auto *callbackPtr = new std::function<void(std::shared_ptr<LFData>)>(callback);
                attr.userData = callbackPtr;

                // 成功回调
                attr.onsuccess = [](emscripten_fetch_t *fetch) {
                    auto *cb = static_cast<std::function<void(
                            std::shared_ptr<LFData>)> *>(fetch->userData);

                    auto data = std::make_shared<LFData>();
                    data->size = fetch->numBytes;
                    data->data = (unsigned char *) malloc(data->size);
                    memcpy(data->data, fetch->data, data->size);

                    (*cb)(data); // 调用引擎的回调

                    delete cb; // 清理 callback wrapper
                    emscripten_fetch_close(fetch); // 释放 fetch 资源
                };

                // 失败回调
                attr.onerror = [](emscripten_fetch_t *fetch) {
                    auto *cb = static_cast<std::function<void(
                            std::shared_ptr<LFData>)> *>(fetch->userData);
                    printf("Fetch failed: %s\n", fetch->url);

                    (*cb)(nullptr); // 告诉引擎加载失败

                    delete cb;
                    emscripten_fetch_close(fetch);
                };

                // 发起请求
                emscripten_fetch(&attr, path.c_str());
            }
    );

    LFEngine::getInstance().setRoot(createAppRoot());

    emscripten_set_mousedown_callback(CANVAS_ID, nullptr, EM_TRUE, mouse_callback);
    emscripten_set_mouseup_callback(CANVAS_ID, nullptr, EM_TRUE, mouse_callback);
    emscripten_set_mousemove_callback(CANVAS_ID, nullptr, EM_TRUE, mouse_callback);

    emscripten_set_touchstart_callback(CANVAS_ID, nullptr, EM_TRUE, touch_callback);
    emscripten_set_touchend_callback(CANVAS_ID, nullptr, EM_TRUE, touch_callback);
    emscripten_set_touchmove_callback(CANVAS_ID, nullptr, EM_TRUE, touch_callback);
    emscripten_set_touchcancel_callback(CANVAS_ID, nullptr, EM_TRUE, touch_callback);

    update_canvas_size();
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_FALSE, on_resize);

    emscripten_set_main_loop(loop_callback, 0, 1);
}
