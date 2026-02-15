#include <jni.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include "LFEngine.h"

static float g_density = 1.0f;

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
Java_net_chentong_leaf_android_LeafRenderer_nativeOnSurfaceCreated(JNIEnv* env, jobject thiz, jobject asset_mgr) {
    AAssetManager* mgr = AAssetManager_fromJava(env, asset_mgr);
    auto asset_loader = [mgr](const char* path) -> std::vector<unsigned char>{
        return read_asset(mgr, path);
    };
    leaf_init(asset_loader);
    std::string js_code = "js_code";
    if (!js_code.empty()) {
        leaf_eval_js(js_code.c_str());
    }
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
