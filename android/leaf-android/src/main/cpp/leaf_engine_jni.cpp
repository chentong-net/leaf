#include <jni.h>
#include <android/asset_manager_jni.h>
#include <string>
#include <android/log.h>
#include <functional>
#include <vector>
#include <chrono>
#include "event/LFEvent.h"
#include "event/LFEventDispatcher.h"
#include "LFEngine.h"

static float g_density = 1.0f;

// 声明 LFEngine.cpp 中的外部函数
extern "C" {
void leaf_init(std::function<std::string(const char* path)> loader);
void leaf_update_size(float w, float h, float d);
void leaf_render();
void leaf_eval_js(const char* code);
}

// 实现具体的读取逻辑
std::string read_asset_js(AAssetManager* mgr, const char* path) {
    std::string p = path;
    __android_log_print(ANDROID_LOG_INFO, "Leaf", "%s", path);
    if (p.find("./") == 0) p = p.substr(2); // 处理 JS 相对路径

    AAsset* asset = AAssetManager_open(mgr, p.c_str(), AASSET_MODE_BUFFER);
    if (!asset) return "";

    size_t size = AAsset_getLength(asset);
    std::string content(size, '\0');
    AAsset_read(asset, &content[0], size);
    AAsset_close(asset);
    return content;
}

extern "C" JNIEXPORT void JNICALL
Java_net_chentong_leaf_android_LeafRenderer_nativeOnSurfaceCreated(JNIEnv* env, jobject thiz, jobject asset_mgr) {
    AAssetManager* mgr = AAssetManager_fromJava(env, asset_mgr);
    auto asset_loader = [mgr](const char* path) -> std::string {
        return read_asset_js(mgr, path);
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
