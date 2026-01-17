#define NANOVG_GLES3_IMPLEMENTATION

#include "LFDef.h"

static EngineContext *g_engine = nullptr;

// JS API: print
static JSValue js_print(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    for (int i = 0; i < argc; i++) {
        const char *str = JS_ToCString(ctx, argv[i]);
        if (str) {
            LF_LOGI("JS_LOG: %s", str);
            JS_FreeCString(ctx, str);
        }
    }
    return JS_UNDEFINED;
}

// JS API: native_request_animation_frame(callback)
static JSValue
js_request_animation_frame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc > 0 && JS_IsFunction(ctx, argv[0])) {
        // 如果之前已经存了一个回调，先释放它的引用计数
        if (!JS_IsUndefined(g_engine->frame_callback)) {
            JS_FreeValue(ctx, g_engine->frame_callback);
        }
        // 增加新回调的引用计数，防止被QuickJS的GC回收
        g_engine->frame_callback = JS_DupValue(ctx, argv[0]);
        return JS_NULL;
    }
    return JS_EXCEPTION;
}

// JS API: nativeDraw(type, x, y, w, h, color)
static JSValue js_native_draw(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc < 2) return JS_UNDEFINED;

    RenderCommand cmd;
    int type_int;
    JS_ToInt32(ctx, &type_int, argv[0]);
    cmd.type = (CommandType) type_int;

    JSValue js_props = argv[1];
    JSPropertyEnum *tab_prop;
    uint32_t len;

    if (JS_GetOwnPropertyNames(ctx, &tab_prop, &len, js_props, JS_GPN_STRING_MASK) >= 0) {
        for (uint32_t i = 0; i < len; i++) {
            const char *key = JS_AtomToCString(ctx, tab_prop[i].atom);
            JSValue val = JS_GetProperty(ctx, js_props, tab_prop[i].atom);

            // 根据 JS 类型自动存入 Map
            if (JS_IsNumber(val)) {
                // 关键点：优先尝试获取为 uint32_t (针对颜色值)
                uint32_t u32_val;
                // JS_ToUint32 是 QuickJS 提供的 API，能正确处理 0xFFFFFFFF 这种位模式
                if (JS_ToUint32(ctx, &u32_val, val) == 0) {
                    // 如果是像 0xFFFFFFFF 这种大整数，或者是明确的整数
                    // 我们需要判断它是否真的应该存为整数。
                    // 一个简单的策略：如果它是颜色 key，或者没有小数部分
                    double d_val;
                    JS_ToFloat64(ctx, &d_val, val);

                    if (d_val == (double) u32_val) {
                        cmd.props[key] = u32_val; // 存入 uint32_t 分支
                    } else {
                        cmd.props[key] = d_val;   // 存入 double 分支
                    }
                } else {
                    double d;
                    JS_ToFloat64(ctx, &d, val);
                    cmd.props[key] = d;
                }
            } else if (JS_IsString(val)) {
                const char *str = JS_ToCString(ctx, val);
                cmd.props[key] = std::string(str);
                JS_FreeCString(ctx, str);
            }
            // 可以继续扩展 bool, uint32 等

            JS_FreeValue(ctx, val);
            JS_FreeCString(ctx, key);
            JS_FreeAtom(ctx, tab_prop[i].atom);
        }
        js_free(ctx, tab_prop);
    }

    g_engine->command_queue.push_back(std::move(cmd));
    return JS_UNDEFINED;
}

// JS API: nativeMeasureText(text, fontSize)
static JSValue
js_measure_text(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc < 2) return JS_EXCEPTION;
    const char *text = JS_ToCString(ctx, argv[0]);
    double fontSize;
    JS_ToFloat64(ctx, &fontSize, argv[1]);

    float bounds[4];
    nvgFontSize(g_engine->vg, (float) fontSize);
    nvgFontFace(g_engine->vg, "sans");
    nvgTextBounds(g_engine->vg, 0, 0, text, nullptr, bounds);

    JS_FreeCString(ctx, text);

    // 返回对象 {width, height}
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "width", JS_NewFloat64(ctx, bounds[2] - bounds[0]));
    JS_SetPropertyStr(ctx, obj, "height", JS_NewFloat64(ctx, bounds[3] - bounds[1]));
    return obj;
}

template<typename T>
T get_prop(const RenderCommand &cmd, const std::string &key, T default_val) {
    auto it = cmd.props.find(key);
    if (it == cmd.props.end()) return default_val;

    return std::visit([default_val](auto &&arg) -> T {
        using StoredType = std::decay_t<decltype(arg)>;


        if constexpr (std::is_same_v<T, StoredType>) { // 如果类型完全匹配（如 std::string）
            return arg;
        } else if constexpr (std::is_arithmetic_v<T>) { // 如果请求的是数值类型 (float, double, uint32_t 等)
            if constexpr (std::is_arithmetic_v<StoredType>) { // 如果存储的也是数值类型 (uint32_t 或 double)
                return static_cast<T>(arg);
            }
            return default_val;
        }
        return default_val;
    }, it->second);
}

static JSModuleDef *js_module_loader(JSContext *ctx, const char *module_name, void *opaque) {
    if (!g_engine->asset_loader) return NULL;

    // 调用我们在 JNI 注入的函数
    std::string source = g_engine->asset_loader(module_name);

    if (source.empty()) {
        JS_ThrowReferenceError(ctx, "Could not load module: %s", module_name);
        return NULL;
    }

    // 编译模块
    JSValue func_val = JS_Eval(ctx, source.c_str(), source.size(), module_name,
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(func_val)) return NULL;

    JSModuleDef *m = (JSModuleDef *) JS_VALUE_GET_PTR(func_val);
    JS_FreeValue(ctx, func_val);
    return m;
}

int get_or_create_image(NVGcontext *vg, const char *path) {
    if (g_image_cache.count(path)) return g_image_cache[path];

    // 调用之前定义的 js_loader 获取资源内容
    std::string data = g_engine->asset_loader(path);
    if (data.empty()) return -1;

    // 创建 NanoVG 图片（从内存加载）
    int img = nvgCreateImageMem(vg, 0, (unsigned char *) data.data(), data.size());
    g_image_cache[path] = img;
    return img;
}

// 暴露给JNI的接口
extern "C" {

void sengine_init(AssetLoaderFunc loader) {
    if (g_engine) return;
    g_engine = new EngineContext();
    g_engine->asset_loader = loader;

    // 初始化JS Runtime
    g_engine->rt = JS_NewRuntime();
    JS_SetModuleLoaderFunc(g_engine->rt, NULL, js_module_loader, NULL);
    g_engine->ctx = JS_NewContext(g_engine->rt);

    // JS API
    JSValue global_obj = JS_GetGlobalObject(g_engine->ctx);
    // print
    JS_SetPropertyStr(g_engine->ctx, global_obj, "print",
                      JS_NewCFunction(g_engine->ctx, js_print, "print", 1));
    // nativeRequestAnimationFrame
    JS_SetPropertyStr(g_engine->ctx, global_obj, "nativeRequestAnimationFrame",
                      JS_NewCFunction(g_engine->ctx, js_request_animation_frame,
                                      "nativeRequestAnimationFrame", 1));
    JS_SetPropertyStr(g_engine->ctx, global_obj, "nativeDraw",
                      JS_NewCFunction(g_engine->ctx, js_native_draw, "nativeDraw", 6));
    JS_SetPropertyStr(g_engine->ctx, global_obj, "nativeMeasureText",
                      JS_NewCFunction(g_engine->ctx, js_measure_text, "nativeMeasureText", 2));
    JS_FreeValue(g_engine->ctx, global_obj);

    // Initialize NanoVG (init with render thread)
    g_engine->vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    LF_LOGI("SEngine Core Initialized");

    if (nvgCreateFont(g_engine->vg, "sans", "/system/fonts/Roboto-Regular.ttf") == -1) {
        // If failed
        nvgCreateFont(g_engine->vg, "sans", "/system/fonts/NotoSansCJK-Regular.ttc");
    }
}

void sengine_update_size(int w, int h, float d) {
    if (g_engine) {
        g_engine->width = w;
        g_engine->height = h;
        g_engine->density = d;
        JSValue global_obj = JS_GetGlobalObject(g_engine->ctx);
        JSValue func = JS_GetPropertyStr(g_engine->ctx, global_obj, "onAppStart");
        if (JS_IsFunction(g_engine->ctx, func)) {
            JSValue args[3];
            args[0] = JS_NewInt32(g_engine->ctx, g_engine->width);
            args[1] = JS_NewInt32(g_engine->ctx, g_engine->height);
            args[2] = JS_NewFloat64(g_engine->ctx, g_engine->density);
            JSValue ret = JS_Call(g_engine->ctx, func, JS_UNDEFINED, 3, args);
            if (JS_IsException(ret)) {
                JSValue exp = JS_GetException(g_engine->ctx);
                const char *msg = JS_ToCString(g_engine->ctx, exp);
                LF_LOGI("JS_ERR: %s", msg);
                JS_FreeCString(g_engine->ctx, msg);
                JS_FreeValue(g_engine->ctx, exp);
            }
            JS_FreeValue(g_engine->ctx, ret);
        }
        JS_FreeValue(g_engine->ctx, global_obj);
        JS_FreeValue(g_engine->ctx, func);
    }
}

void sengine_eval_js(const char *code) {
    if (!g_engine || !code) return;
    // 执行main.js
    JSValue val = JS_Eval(g_engine->ctx, code, strlen(code), "main.js", JS_EVAL_TYPE_MODULE);
    if (JS_IsException(val)) {
        JSValue exception = JS_GetException(g_engine->ctx);
        const char *msg = JS_ToCString(g_engine->ctx, exception);
        LF_LOGI("JS_ERR: %s", msg);
        JS_FreeCString(g_engine->ctx, msg);
        JS_FreeValue(g_engine->ctx, exception);
    }
    JS_FreeValue(g_engine->ctx, val);
}

void sengine_render() {
    if (!g_engine || !g_engine->vg) return;

    // Frame Callback
    if (!JS_IsUndefined(g_engine->frame_callback)) {
        // Call JS
        JSValue ret = JS_Call(g_engine->ctx, g_engine->frame_callback, JS_UNDEFINED, 0, nullptr);
        if (JS_IsException(ret)) {
            JSValue exp = JS_GetException(g_engine->ctx);
            const char *msg = JS_ToCString(g_engine->ctx, exp);
            LF_LOGI("JS_ERR: %s", msg);
            JS_FreeCString(g_engine->ctx, msg);
            JS_FreeValue(g_engine->ctx, exp);
        }
        JS_FreeValue(g_engine->ctx, ret);
    }

    nvgBeginFrame(g_engine->vg, g_engine->width, g_engine->height, g_engine->density);

    auto to_nvg_color = [](uint32_t c) {
        return nvgRGBA((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF);
    };

    for (const auto &cmd: g_engine->command_queue) {
        float x = get_prop(cmd, "x", 0.0f);
        float y = get_prop(cmd, "y", 0.0f);
        float w = get_prop(cmd, "w", 0.0f);
        float h = get_prop(cmd, "h", 0.0f);
        uint32_t color_val = get_prop<uint32_t>(cmd, "color", 0xFFFFFFFFU);
        NVGcolor color = to_nvg_color(color_val);
        LF_LOGI("Color Val: %u, R: %d, G: %d, B: %d", color_val, (color_val >> 16) & 0xFF,
                (color_val >> 8) & 0xFF, color_val & 0xFF);

        switch (cmd.type) {
            case CMD_RECT:
                nvgBeginPath(g_engine->vg);
                nvgRoundedRect(g_engine->vg, x, y, w, h, get_prop(cmd, "radius", 0.0f));
                nvgFillColor(g_engine->vg, to_nvg_color(color_val));
                nvgFill(g_engine->vg);
                break;
            case CMD_CIRCLE:
                nvgBeginPath(g_engine->vg);
                // 对于圆，w通常作为半径radius
                nvgCircle(g_engine->vg, x, y, get_prop(cmd, "radius", w));
                nvgFillColor(g_engine->vg, color);
                nvgFill(g_engine->vg);
                break;
            case CMD_TEXT:
                nvgFontSize(g_engine->vg, get_prop(cmd, "fontSize", 12.0f));
                nvgFontFace(g_engine->vg, "sans");
                nvgFillColor(g_engine->vg, color);
                nvgTextAlign(g_engine->vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
                nvgText(g_engine->vg, x, y, get_prop(cmd, "text", std::string("")).c_str(),
                        nullptr);
                break;
            case CMD_CLEAR:
                nvgBeginPath(g_engine->vg);
                nvgRoundedRect(g_engine->vg, 0, 0, g_engine->width, g_engine->height, 0);
                nvgFillColor(g_engine->vg, to_nvg_color(0xFFFFFFFF));
                nvgFill(g_engine->vg);
                break;
            case CMD_IMAGE:
                std::string path = get_prop(cmd, "path", std::string(""));
                int img = get_or_create_image(g_engine->vg, path.c_str());
                if (img > 0) {
                    int imgW, imgH;
                    nvgImageSize(g_engine->vg, img, &imgW, &imgH);
                    NVGpaint imgPaint = nvgImagePattern(g_engine->vg, x, y, w, h, 0, img, 1.0f);
                    nvgBeginPath(g_engine->vg);
                    nvgRoundedRect(g_engine->vg, x, y, w, h, get_prop(cmd, "radius", 0.0f));
                    nvgFillPaint(g_engine->vg, imgPaint);
                    nvgFill(g_engine->vg);
                }
                break;
        }
    }

    // 绘制完成，清空本帧指令
    g_engine->command_queue.clear();
    nvgEndFrame(g_engine->vg);
}

} // extern "C"