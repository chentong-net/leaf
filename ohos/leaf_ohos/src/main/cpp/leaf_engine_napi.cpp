#include "napi/native_api.h"
#include <rawfile/raw_file_manager.h>
#include "LFEngine.h"
#include "leaf_file_service_napi.h"
#include <thread>
#include <atomic>
#include <mutex>

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

static napi_value Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"initEngine", nullptr, InitEngine, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_properties(env, exports, 1, desc);
    leafRegisterFileServiceNapi(env, exports);

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
