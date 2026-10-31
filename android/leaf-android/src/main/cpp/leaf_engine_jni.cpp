#include <jni.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <cstdint>
#include <string>
#include <vector>
#include "LFEngine.h"
#include "plugin/LFNativeSender.h"

static float g_density = 1.0f;
static JavaVM* g_javaVm = nullptr;
static jobject g_pluginDispatcherObj = nullptr;
static jmethodID g_pluginDispatchMethod = nullptr;
static std::mutex g_pluginBridgeMutex;
static jweak g_leafRendererObj = nullptr;
static jmethodID g_textInputCursorMethod = nullptr;
static std::mutex g_rendererBridgeMutex;

namespace {

LFKeyCode toLFKeyCode(jint rawKeyCode) {
    switch (rawKeyCode) {
        case 13:
            return LFKeyCode::Enter;
        case 9:
            return LFKeyCode::Tab;
        case 8:
            return LFKeyCode::Backspace;
        case 27:
            return LFKeyCode::Escape;
        case 127:
            return LFKeyCode::Delete;
        case 1001:
            return LFKeyCode::Left;
        case 1002:
            return LFKeyCode::Right;
        case 1003:
            return LFKeyCode::Up;
        case 1004:
            return LFKeyCode::Down;
        case 1005:
            return LFKeyCode::Home;
        case 1006:
            return LFKeyCode::End;
        default:
            return LFKeyCode::Unknown;
    }
}

LFKeyEventType toLFKeyEventType(jint rawType) {
    return rawType == 1 ? LFKeyEventType::Up : LFKeyEventType::Down;
}

void appendUtf8(std::string& output, uint32_t codepoint) {
    if (codepoint <= 0x7F) {
        output.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        output.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
        output.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

void appendUtf16(std::vector<jchar>& output, uint32_t codepoint) {
    if (codepoint <= 0xFFFF) {
        output.push_back(static_cast<jchar>(codepoint));
        return;
    }

    codepoint -= 0x10000;
    output.push_back(static_cast<jchar>(0xD800 | (codepoint >> 10)));
    output.push_back(static_cast<jchar>(0xDC00 | (codepoint & 0x3FF)));
}

std::string toStdString(JNIEnv* env, jstring value) {
    if (!value) return "";

    const jsize length = env->GetStringLength(value);
    if (length <= 0) return "";

    std::vector<jchar> utf16(static_cast<size_t>(length));
    env->GetStringRegion(value, 0, length, utf16.data());
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return "";
    }

    std::string result;
    result.reserve(static_cast<size_t>(length));
    for (jsize i = 0; i < length; ++i) {
        const uint32_t unit = utf16[static_cast<size_t>(i)];
        if (unit >= 0xD800 && unit <= 0xDBFF) {
            if (i + 1 < length) {
                const uint32_t low = utf16[static_cast<size_t>(i + 1)];
                if (low >= 0xDC00 && low <= 0xDFFF) {
                    appendUtf8(
                            result,
                            0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00)
                    );
                    ++i;
                    continue;
                }
            }
            appendUtf8(result, 0xFFFD);
        } else if (unit >= 0xDC00 && unit <= 0xDFFF) {
            appendUtf8(result, 0xFFFD);
        } else {
            appendUtf8(result, unit);
        }
    }
    return result;
}

jstring toJString(JNIEnv* env, const std::string& value) {
    std::vector<jchar> utf16;
    utf16.reserve(value.size());

    size_t offset = 0;
    while (offset < value.size()) {
        const uint8_t first = static_cast<uint8_t>(value[offset]);
        uint32_t codepoint = 0xFFFD;
        size_t sequenceLength = 1;

        if (first <= 0x7F) {
            codepoint = first;
        } else if (first >= 0xC2 && first <= 0xDF &&
                   offset + 1 < value.size()) {
            const uint8_t second = static_cast<uint8_t>(value[offset + 1]);
            if ((second & 0xC0) == 0x80) {
                codepoint = ((first & 0x1F) << 6) | (second & 0x3F);
                sequenceLength = 2;
            }
        } else if (first >= 0xE0 && first <= 0xEF &&
                   offset + 2 < value.size()) {
            const uint8_t second = static_cast<uint8_t>(value[offset + 1]);
            const uint8_t third = static_cast<uint8_t>(value[offset + 2]);
            if ((second & 0xC0) == 0x80 && (third & 0xC0) == 0x80) {
                const uint32_t candidate =
                        ((first & 0x0F) << 12) |
                        ((second & 0x3F) << 6) |
                        (third & 0x3F);
                if (candidate >= 0x800 &&
                    !(candidate >= 0xD800 && candidate <= 0xDFFF)) {
                    codepoint = candidate;
                    sequenceLength = 3;
                }
            }
        } else if (first >= 0xF0 && first <= 0xF4 &&
                   offset + 3 < value.size()) {
            const uint8_t second = static_cast<uint8_t>(value[offset + 1]);
            const uint8_t third = static_cast<uint8_t>(value[offset + 2]);
            const uint8_t fourth = static_cast<uint8_t>(value[offset + 3]);
            if ((second & 0xC0) == 0x80 &&
                (third & 0xC0) == 0x80 &&
                (fourth & 0xC0) == 0x80) {
                const uint32_t candidate =
                        ((first & 0x07) << 18) |
                        ((second & 0x3F) << 12) |
                        ((third & 0x3F) << 6) |
                        (fourth & 0x3F);
                if (candidate >= 0x10000 && candidate <= 0x10FFFF) {
                    codepoint = candidate;
                    sequenceLength = 4;
                }
            }
        }

        appendUtf16(utf16, codepoint);
        offset += sequenceLength;
    }

    if (utf16.empty()) {
        const jchar empty = 0;
        return env->NewString(&empty, 0);
    }
    return env->NewString(utf16.data(), static_cast<jsize>(utf16.size()));
}

JNIEnv* getJNIEnv(bool* didAttach) {
    if (didAttach) *didAttach = false;
    if (!g_javaVm) return nullptr;

    JNIEnv* env = nullptr;
    jint status = g_javaVm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (status == JNI_OK) {
        return env;
    }
    if (status != JNI_EDETACHED) {
        return nullptr;
    }
    if (g_javaVm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        return nullptr;
    }
    if (didAttach) *didAttach = true;
    return env;
}

void resolveBridgeError(int32_t requestId, int32_t code, const char* error) {
    LFMethodResult result;
    result.requestId = requestId;
    result.ok = false;
    result.code = code;
    result.error = error ? error : "bridge_error";
    LFNativeSender::getInstance().resolve(result);
}

void dispatchMethodCallToJava(const LFMethodCall& call) {
    bool didAttach = false;
    JNIEnv* env = getJNIEnv(&didAttach);
    if (!env) {
        resolveBridgeError(call.requestId, -10, "jni_env_unavailable");
        return;
    }

    jobject dispatcherObj = nullptr;
    jmethodID dispatchMethod = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_pluginBridgeMutex);
        dispatcherObj = g_pluginDispatcherObj;
        dispatchMethod = g_pluginDispatchMethod;
    }

    if (!dispatcherObj || !dispatchMethod) {
        resolveBridgeError(call.requestId, -11, "plugin_dispatcher_unavailable");
        if (didAttach) {
            g_javaVm->DetachCurrentThread();
        }
        return;
    }

    jstring method = toJString(env, call.method);
    jstring args = toJString(env, call.args);
    if (!method || !args) {
        if (method) env->DeleteLocalRef(method);
        if (args) env->DeleteLocalRef(args);
        resolveBridgeError(call.requestId, -13, "jni_string_allocation_failed");
        if (didAttach) {
            g_javaVm->DetachCurrentThread();
        }
        return;
    }
    env->CallVoidMethod(dispatcherObj, dispatchMethod, method, static_cast<jint>(call.requestId), args);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        resolveBridgeError(call.requestId, -12, "plugin_dispatcher_throw");
    }

    if (method) env->DeleteLocalRef(method);
    if (args) env->DeleteLocalRef(args);
    if (didAttach) {
        g_javaVm->DetachCurrentThread();
    }
}

bool initPluginBridge(JNIEnv* env) {
    if (!env) return false;

    std::lock_guard<std::mutex> lock(g_pluginBridgeMutex);
    if (g_pluginDispatcherObj && g_pluginDispatchMethod) {
        return true;
    }

    jclass dispatcherCls = env->FindClass("net/chentong/leaf/android/PluginDispatcher");
    if (!dispatcherCls) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        return false;
    }

    jmethodID getInstance = env->GetStaticMethodID(
        dispatcherCls,
        "getInstance",
        "()Lnet/chentong/leaf/android/PluginDispatcher;"
    );
    jmethodID dispatch = env->GetMethodID(dispatcherCls, "dispatch", "(Ljava/lang/String;ILjava/lang/String;)V");
    if (!getInstance || !dispatch) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteLocalRef(dispatcherCls);
        return false;
    }

    jobject dispatcherObjLocal = env->CallStaticObjectMethod(dispatcherCls, getInstance);
    if (!dispatcherObjLocal || env->ExceptionCheck()) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteLocalRef(dispatcherCls);
        return false;
    }

    g_pluginDispatcherObj = env->NewGlobalRef(dispatcherObjLocal);
    g_pluginDispatchMethod = dispatch;

    env->DeleteLocalRef(dispatcherObjLocal);
    env->DeleteLocalRef(dispatcherCls);

    LFNativeSender::getInstance().bindTarget(dispatchMethodCallToJava);
    return true;
}

}

// 声明 engine_bridge.cpp 中的外部函数
extern "C" {
void leaf_init(std::function<std::vector<unsigned char>(const char* path)> loader);
void leaf_update_size(float w, float h, float d);
void leaf_render();
void leaf_eval_js(const char* code);
}

// 实现具体的读取逻辑
std::vector<unsigned char> read_asset(AAssetManager* mgr, const char* path) {
    std::string p = path;
    LF_LOGI("read asset: %s", path);
    if (p.find("./") == 0) p = p.substr(2);

    AAsset* asset = AAssetManager_open(mgr, p.c_str(), AASSET_MODE_BUFFER);
    if (!asset) return std::vector<unsigned char>();

    size_t size = AAsset_getLength(asset);
    std::vector<unsigned char> content(size);
    AAsset_read(asset, content.data(), size);
    AAsset_close(asset);
    return content;
}

extern "C" JNIEXPORT void JNICALL
Java_net_chentong_leaf_android_PluginDispatcher_nativeNotifyPluginResult(
    JNIEnv* env,
    jclass clazz,
    jint requestId,
    jboolean ok,
    jint code,
    jboolean canceled,
    jstring data,
    jstring error
) {
    LFMethodResult result;
    result.requestId = static_cast<int32_t>(requestId);
    result.ok = ok == JNI_TRUE;
    result.canceled = canceled == JNI_TRUE;
    result.code = static_cast<int32_t>(code);
    result.data = toStdString(env, data);
    result.error = toStdString(env, error);
    LFNativeSender::getInstance().resolve(result);
}

extern "C" JNIEXPORT void JNICALL
Java_net_chentong_leaf_android_LeafRenderer_nativeOnSurfaceCreated(JNIEnv* env, jobject thiz, jobject asset_mgr) {
    {
        std::lock_guard<std::mutex> lock(g_rendererBridgeMutex);
        if (g_leafRendererObj) {
            env->DeleteWeakGlobalRef(g_leafRendererObj);
            g_leafRendererObj = nullptr;
        }

        jclass rendererClass = env->GetObjectClass(thiz);
        g_textInputCursorMethod = env->GetMethodID(
                rendererClass,
                "onNativeTextInputCursorChanged",
                "(FFF)V"
        );
        if (g_textInputCursorMethod) {
            g_leafRendererObj = env->NewWeakGlobalRef(thiz);
        } else if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
        env->DeleteLocalRef(rendererClass);
    }

    initPluginBridge(env);
    AAssetManager* mgr = AAssetManager_fromJava(env, asset_mgr);
    auto asset_loader = [mgr](const char* path) -> std::vector<unsigned char>{
        return read_asset(mgr, path);
    };
    leaf_init(asset_loader);
    LFEngine::getInstance().setTextInputCursorCallback([](float x, float y, float lineHeight) {
        bool didAttach = false;
        JNIEnv* callbackEnv = getJNIEnv(&didAttach);
        if (!callbackEnv) {
            return;
        }

        jobject renderer = nullptr;
        jmethodID method = nullptr;
        {
            std::lock_guard<std::mutex> lock(g_rendererBridgeMutex);
            method = g_textInputCursorMethod;
            if (g_leafRendererObj && method) {
                renderer = callbackEnv->NewLocalRef(g_leafRendererObj);
            }
        }

        if (renderer && method) {
            const jfloat density = g_density > 0.0f ? g_density : 1.0f;
            callbackEnv->CallVoidMethod(
                    renderer,
                    method,
                    x * density,
                    y * density,
                    lineHeight * density
            );
            if (callbackEnv->ExceptionCheck()) {
                callbackEnv->ExceptionDescribe();
                callbackEnv->ExceptionClear();
            }
            callbackEnv->DeleteLocalRef(renderer);
        }

        if (didAttach) {
            g_javaVm->DetachCurrentThread();
        }
    });
    std::string js_code = "js_code";
    if (!js_code.empty()) {
        leaf_eval_js(js_code.c_str());
    }
}

extern "C" JNIEXPORT void JNICALL
Java_net_chentong_leaf_android_LeafRenderer_nativeRelease(JNIEnv* env, jobject thiz) {
    std::lock_guard<std::mutex> lock(g_rendererBridgeMutex);
    if (g_leafRendererObj) {
        env->DeleteWeakGlobalRef(g_leafRendererObj);
        g_leafRendererObj = nullptr;
    }
    g_textInputCursorMethod = nullptr;
    LFEngine::getInstance().discardContext();
}

extern "C" JNIEXPORT void JNICALL
Java_net_chentong_leaf_android_LeafRenderer_nativeOnSurfaceChanged(JNIEnv* env, jobject thiz, jint w, jint h, jfloat d) {
    g_density = d;
    leaf_update_size(w / d, h / d, d);
}

extern "C" JNIEXPORT void JNICALL
Java_net_chentong_leaf_android_LeafRenderer_nativeOnDrawFrame(JNIEnv* env, jobject thiz) {
    leaf_render();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_net_chentong_leaf_android_LeafRenderer_nativeIsTextInputFocused(JNIEnv* env, jobject thiz) {
    return LFEventDispatcher::getInstance().getFocusedNode() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_net_chentong_leaf_android_LeafRenderer_nativeDispatchKeyEvent(
    JNIEnv* env, jobject thiz, jint type, jint keyCode, jint modifiers, jboolean repeat
) {
    LFEventDispatcher::getInstance().dispatchKeyEvent(
        toLFKeyEventType(type),
        toLFKeyCode(keyCode),
        static_cast<uint32_t>(modifiers),
        repeat == JNI_TRUE
    );
}

extern "C" JNIEXPORT void JNICALL
Java_net_chentong_leaf_android_LeafRenderer_nativeDispatchCharInput(
    JNIEnv* env, jobject thiz, jint codepoint
) {
    if (codepoint <= 0) return;
    LFEventDispatcher::getInstance().dispatchCharInput(static_cast<uint32_t>(codepoint));
}

extern "C" JNIEXPORT void JNICALL
Java_net_chentong_leaf_android_LeafRenderer_nativeDispatchTouchEvent(
    JNIEnv* env, jobject thiz,
    jint action,
    jint actionIndex,
    jint pointerCount,
    jintArray pointerIds,
    jfloatArray x,
    jfloatArray y,
    jfloatArray pressure,
    jlong eventTime
) {
    // 1. Convert Java arrays to C++ arrays
    jint* ids = env->GetIntArrayElements(pointerIds, nullptr);
    jfloat* xArr = env->GetFloatArrayElements(x, nullptr);
    jfloat* yArr = env->GetFloatArrayElements(y, nullptr);
    jfloat* pressureArr = env->GetFloatArrayElements(pressure, nullptr);

    // 2. Build touch point list
    std::vector<LFTouchPoint> touches;
    std::vector<LFTouchID> changedIDs;

    // Use engine's elapsed time for consistent time base
    double timestampSeconds = LFEngine::getInstance().getElapsedTime();

    for (int i = 0; i < pointerCount; i++) {
        LFTouchPoint touch;
        touch.id = ids[i];
        touch.x = xArr[i] / g_density;
        touch.y = yArr[i] / g_density;
        touch.pressure = pressureArr[i];
        touch.timestamp = timestampSeconds;
        touches.push_back(touch);
    }

    // 3. Determine event type and changed touch points
    LFTouchEventType type;

    switch (action & 0xFF) {  // ACTION_MASK
        case 0:  // MotionEvent.ACTION_DOWN
            type = LFTouchEventType::Down;
            changedIDs.push_back(ids[actionIndex]);
            break;

        case 5:  // MotionEvent.ACTION_POINTER_DOWN
            type = LFTouchEventType::Down;
            changedIDs.push_back(ids[actionIndex]);
            break;

        case 2:  // MotionEvent.ACTION_MOVE
            type = LFTouchEventType::Move;
            // MOVE affects all touch points
            for (int i = 0; i < pointerCount; i++) {
                changedIDs.push_back(ids[i]);
            }
            break;

        case 1:  // MotionEvent.ACTION_UP
            type = LFTouchEventType::Up;
            actionIndex = 0;
            changedIDs.push_back(ids[actionIndex]);
            break;

        case 6:  // MotionEvent.ACTION_POINTER_UP
            type = LFTouchEventType::Up;
            changedIDs.push_back(ids[actionIndex]);
            break;

        case 3:  // MotionEvent.ACTION_CANCEL
            type = LFTouchEventType::Cancel;
            for (int i = 0; i < pointerCount; i++) {
                changedIDs.push_back(ids[i]);
            }
            break;

        default:
            // Unknown action, release arrays and return
            env->ReleaseIntArrayElements(pointerIds, ids, 0);
            env->ReleaseFloatArrayElements(x, xArr, 0);
            env->ReleaseFloatArrayElements(y, yArr, 0);
            env->ReleaseFloatArrayElements(pressure, pressureArr, 0);
            return;
    }

    // 4. Dispatch to event system
    auto& dispatcher = LFEventDispatcher::getInstance();
    auto root = LFEngine::getInstance().getRoot();

    if (root) {
        dispatcher.dispatchTouchEvent(type, touches, changedIDs, root);
    }

    // 5. Release arrays
    env->ReleaseIntArrayElements(pointerIds, ids, 0);
    env->ReleaseFloatArrayElements(x, xArr, 0);
    env->ReleaseFloatArrayElements(y, yArr, 0);
    env->ReleaseFloatArrayElements(pressure, pressureArr, 0);
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_javaVm = vm;
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    initPluginBridge(env);
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_pluginBridgeMutex);
    if (g_pluginDispatcherObj) {
        env->DeleteGlobalRef(g_pluginDispatcherObj);
        g_pluginDispatcherObj = nullptr;
    }
    g_pluginDispatchMethod = nullptr;
    LFNativeSender::getInstance().bindTarget(nullptr);
    LFNativeSender::getInstance().clearPending();
    {
        std::lock_guard<std::mutex> rendererLock(g_rendererBridgeMutex);
        if (g_leafRendererObj) {
            env->DeleteWeakGlobalRef(g_leafRendererObj);
            g_leafRendererObj = nullptr;
        }
        g_textInputCursorMethod = nullptr;
    }
}
