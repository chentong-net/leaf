#include "napi/native_api.h"
#include <rawfile/raw_file_manager.h>
#include "LFEngine.h"
#include "plugin/LFNativeSender.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>

extern "C" {
void leaf_init(std::function<std::vector<unsigned char>(const char *path)> loader);
void leaf_update_size(float w, float h, float d);
void leaf_render();
void leaf_eval_js(const char* code);
}

NativeResourceManager* g_resMgr = nullptr;
OHNativeWindow* g_window = nullptr;

// 线程控制
std::thread* g_renderThread = nullptr;
std::atomic<bool> g_isRunning = false;
std::atomic<bool> g_isWindowChanged = true;

// 屏幕信息
std::atomic<int> g_width = 0; // dp
std::atomic<int> g_height = 0; // dp
std::atomic<float> g_density = 1.0f;

std::mutex g_pluginBridgeMutex;
napi_threadsafe_function g_pluginDispatchTsfn = nullptr;

namespace {

struct PendingMethodCall {
    int32_t requestId = 0;
    std::string method;
    std::string args;
};

std::string ToStdString(napi_env env, napi_value value) {
    if (!env || !value) {
        return "";
    }
    size_t length = 0;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) {
        return "";
    }
    std::string out(length + 1, '\0');
    if (length == 0) {
        return "";
    }
    size_t copied = 0;
    if (napi_get_value_string_utf8(env, value, out.data(), out.size(), &copied) != napi_ok) {
        return "";
    }
    out.resize(copied);
    return out;
}

void ResolveBridgeError(int32_t requestId, int32_t code, const char* error) {
    LFMethodResult result;
    result.requestId = requestId;
    result.ok = false;
    result.code = code;
    result.error = error ? error : "bridge_error";
    LFNativeSender::getInstance().resolve(result);
}

void CallPluginDispatcher(napi_env env, napi_value jsCallback, void* context, void* data) {
    std::unique_ptr<PendingMethodCall> call(static_cast<PendingMethodCall*>(data));
    if (!call) {
        return;
    }
    if (!env || !jsCallback) {
        return;
    }

    napi_value global = nullptr;
    napi_get_global(env, &global);

    napi_value argv[3] = {nullptr, nullptr, nullptr};
    napi_create_string_utf8(env, call->method.c_str(), NAPI_AUTO_LENGTH, &argv[0]);
    napi_create_int32(env, call->requestId, &argv[1]);
    napi_create_string_utf8(env, call->args.c_str(), NAPI_AUTO_LENGTH, &argv[2]);

    napi_value ignored = nullptr;
    napi_status status = napi_call_function(env, global, jsCallback, 3, argv, &ignored);
    if (status != napi_ok) {
        ResolveBridgeError(call->requestId, -12, "ohos_dispatcher_throw");
        bool hasPendingException = false;
        if (napi_is_exception_pending(env, &hasPendingException) == napi_ok && hasPendingException) {
            napi_get_and_clear_last_exception(env, &ignored);
        }
    }
}

void TsfnFinalize(napi_env env, void* finalize_data, void* finalize_hint) {
    {
        std::lock_guard<std::mutex> lock(g_pluginBridgeMutex);
        g_pluginDispatchTsfn = nullptr;
    }
    LFNativeSender::getInstance().bindTarget(nullptr);
    LFNativeSender::getInstance().clearPending();
}

void DispatchMethodCallToArkTS(const LFMethodCall& call) {
    napi_threadsafe_function tsfn = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_pluginBridgeMutex);
        tsfn = g_pluginDispatchTsfn;
    }
    if (!tsfn) {
        ResolveBridgeError(call.requestId, -11, "ohos_bridge_unavailable");
        return;
    }

    PendingMethodCall* pending = new PendingMethodCall();
    pending->requestId = call.requestId;
    pending->method = call.method;
    pending->args = call.args;
    napi_status status = napi_call_threadsafe_function(tsfn, pending, napi_tsfn_nonblocking);
    if (status != napi_ok) {
        delete pending;
        ResolveBridgeError(call.requestId, -13, "ohos_dispatch_enqueue_failed");
    }
}

} // namespace

// 资源读取
std::vector<unsigned char> read_asset(const char* path) {
    if (!g_resMgr) return {};
    RawFile* file = OH_ResourceManager_OpenRawFile(g_resMgr, path);
    if (!file) return {};
    long len = OH_ResourceManager_GetRawFileSize(file);
    std::vector<unsigned char> buf(len);
    OH_ResourceManager_ReadRawFile(file, buf.data(), len);
    OH_ResourceManager_CloseRawFile(file);
    return buf;
}

void RenderThreadLoop() {
    // 初始化 EGL
    // 渲染线程
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_STENCIL_SIZE, 8, // NanoVG 需要
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    // 创建 Surface 和 Context
    EGLSurface surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)g_window, nullptr);
    const EGLint ctxAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttribs);

    // 绑定上下文到本线程
    if (!eglMakeCurrent(display, surface, surface, context)) {
        return; // 出错退出
    }

    // 初始化 Leaf 引擎
    if (g_resMgr) {
        leaf_init(read_asset);
        leaf_update_size(g_width, g_height, g_density);
        std::string js_code = "js_code";
        if (!js_code.empty()) {
            leaf_eval_js(js_code.c_str());
        }
    }

    // 渲染循环
    while (g_isRunning) {
        // 处理窗口大小变化
        if (g_isWindowChanged) {
            leaf_update_size(g_width, g_height, g_density);
            g_isWindowChanged = false;
        }
        
        // 渲染一帧
        leaf_render();

        // 交换缓冲区 (这一步通常会等待 VSync，控制帧率)
        eglSwapBuffers(display, surface);
    }

    // 清理
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
}

// XComponent 回调

void OnSurfaceCreated(OH_NativeXComponent* component, void* window) {
    g_window = (OHNativeWindow*)window;
    
    uint64_t w, h;
    OH_NativeXComponent_GetXComponentSize(component, window, &w, &h);
    
    g_width = (int)(w / g_density);
    g_height = (int)(h / g_density);
    g_isWindowChanged = true;
    
    // 启动渲染线程
    g_isRunning = true;
    g_renderThread = new std::thread(RenderThreadLoop);
}

void OnSurfaceChanged(OH_NativeXComponent* component, void* window) {
    uint64_t w, h;
    OH_NativeXComponent_GetXComponentSize(component, window, &w, &h);
    
    g_width = w / g_density;
    g_height = h / g_density;
    g_isWindowChanged = true;
}

void OnSurfaceDestroyed(OH_NativeXComponent* component, void* window) {
    // 停止线程
    g_isRunning = false;
    if (g_renderThread && g_renderThread->joinable()) {
        g_renderThread->join();
        delete g_renderThread;
        g_renderThread = nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(g_pluginBridgeMutex);
        if (g_pluginDispatchTsfn != nullptr) {
            napi_release_threadsafe_function(g_pluginDispatchTsfn, napi_tsfn_release);
            g_pluginDispatchTsfn = nullptr;
        }
    }
    LFNativeSender::getInstance().bindTarget(nullptr);
    LFNativeSender::getInstance().clearPending();
}

void DispatchTouchEvent(OH_NativeXComponent* component, void* window) {
    OH_NativeXComponent_TouchEvent touchEvent;
    // 从组件中获取详细的触摸信息
    int32_t ret = OH_NativeXComponent_GetTouchEvent(component, window, &touchEvent);
    if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) return;

    // 转换类型
    LFTouchEventType leafType;
    switch (touchEvent.type) {
        case OH_NATIVEXCOMPONENT_DOWN: leafType = LFTouchEventType::Down; break;
        case OH_NATIVEXCOMPONENT_UP: leafType = LFTouchEventType::Up; break;
        case OH_NATIVEXCOMPONENT_MOVE: leafType = LFTouchEventType::Move; break;
        case OH_NATIVEXCOMPONENT_CANCEL: leafType = LFTouchEventType::Cancel; break;
        default: return;
    }

    // 转换坐标 (Native 获取的也是物理像素)
    std::vector<LFTouchPoint> touches;
    std::vector<LFTouchID> changedIDs;
    double timestamp = LFEngine::getInstance().getElapsedTime();

    for (uint32_t i = 0; i < touchEvent.numPoints; i++) {
        LFTouchPoint tp;
        tp.id = touchEvent.touchPoints[i].id;
        tp.x = touchEvent.touchPoints[i].x / g_density; // 统一使用逻辑像素
        tp.y = touchEvent.touchPoints[i].y / g_density;
        tp.pressure = touchEvent.touchPoints[i].force;
        tp.timestamp = timestamp;
        touches.push_back(tp);
        
        // 标记变化的 ID (简单逻辑，复杂情况需根据 type 判断)
        if (i == touchEvent.numPoints - 1) { 
            changedIDs.push_back(tp.id);
        }
    }

    // 分发给引擎
    auto root = LFEngine::getInstance().getRoot();
    if (root) {
        LFEventDispatcher::getInstance().dispatchTouchEvent(leafType, touches, changedIDs, root);
    }
}

EXTERN_C_START
static napi_value InitEngine(napi_env env, napi_callback_info info) {
    // 接收的参数数量
    size_t argc = 4; 
    napi_value args[argc];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc > 0) {
        g_resMgr = OH_ResourceManager_InitNativeResourceManager(env, args[0]);
    }

    // 获取屏幕宽高和像素密度
    double density = 1.0;
    
    if (argc >= 2) {
        // 尝试解析 JS 传来的 number
        napi_status status = napi_get_value_double(env, args[1], &density);
        if (status == napi_ok) {
            g_density = (float)density;
            g_width = g_width / g_density;
            g_height = g_height / g_density;
            int w = g_width;
            int h = g_height;
            LF_LOGI("screen size initialized: width: %{public}d, height: %{public}d, density: %{public}f", w, h, density);
        }
    } else {
        LF_LOGI("argument missing");
    }

    return nullptr;
}

static napi_value RegisterPluginDispatcher(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1) {
        return nullptr;
    }

    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, args[0], &type) != napi_ok || type != napi_function) {
        return nullptr;
    }

    napi_value resourceName = nullptr;
    napi_create_string_utf8(env, "LeafPluginDispatcher", NAPI_AUTO_LENGTH, &resourceName);

    {
        std::lock_guard<std::mutex> lock(g_pluginBridgeMutex);
        if (g_pluginDispatchTsfn != nullptr) {
            napi_release_threadsafe_function(g_pluginDispatchTsfn, napi_tsfn_release);
            g_pluginDispatchTsfn = nullptr;
        }

        napi_status status = napi_create_threadsafe_function(
                env,
                args[0],
                nullptr,
                resourceName,
                0,
                1,
                nullptr,
                TsfnFinalize,
                nullptr,
                CallPluginDispatcher,
                &g_pluginDispatchTsfn
        );
        if (status != napi_ok || g_pluginDispatchTsfn == nullptr) {
            LFNativeSender::getInstance().bindTarget(nullptr);
            LFNativeSender::getInstance().clearPending();
            return nullptr;
        }
    }

    LFNativeSender::getInstance().bindTarget(DispatchMethodCallToArkTS);
    return nullptr;
}

static napi_value NotifyPluginResult(napi_env env, napi_callback_info info) {
    size_t argc = 6;
    napi_value args[6] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 6) {
        return nullptr;
    }

    int32_t requestId = 0;
    bool ok = false;
    int32_t code = 0;
    bool canceled = false;

    napi_get_value_int32(env, args[0], &requestId);
    napi_get_value_bool(env, args[1], &ok);
    napi_get_value_int32(env, args[2], &code);
    napi_get_value_bool(env, args[3], &canceled);

    LFMethodResult result;
    result.requestId = requestId;
    result.ok = ok;
    result.code = code;
    result.canceled = canceled;
    result.data = ToStdString(env, args[4]);
    result.error = ToStdString(env, args[5]);
    LFNativeSender::getInstance().resolve(result);
    return nullptr;
}

static napi_value Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"initEngine", nullptr, InitEngine, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"registerPluginDispatcher", nullptr, RegisterPluginDispatcher, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"notifyPluginResult", nullptr, NotifyPluginResult, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_properties(env, exports, 3, desc);

    napi_value exportInstance = nullptr;
    if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) == napi_ok) {
        OH_NativeXComponent *nativeXComponent = nullptr;
        napi_unwrap(env, exportInstance, reinterpret_cast<void**>(&nativeXComponent));
        
        static OH_NativeXComponent_Callback callback;
        callback.OnSurfaceCreated = OnSurfaceCreated;
        callback.OnSurfaceChanged = OnSurfaceChanged;
        callback.OnSurfaceDestroyed = OnSurfaceDestroyed;
        callback.DispatchTouchEvent = DispatchTouchEvent;
        OH_NativeXComponent_RegisterCallback(nativeXComponent, &callback);
    }
    return exports;
}
EXTERN_C_END

static napi_module leafModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Export,
    .nm_modname = "leaf_ohos", 
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterLeafModule() {
    napi_module_register(&leafModule);
}
