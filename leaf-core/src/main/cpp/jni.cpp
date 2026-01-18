#include <jni.h>
#include <android/asset_manager_jni.h>
#include <string>
#include <android/log.h>
#include <functional>

// 声明 LFEngine.cpp 中的外部函数
extern "C" {
void leaf_init(std::function<std::string(const char* path)> loader);
void leaf_update_size(int w, int h, float d);
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
Java_net_chentong_leaf_core_LeafRenderer_nativeOnSurfaceCreated(JNIEnv* env, jobject thiz, jobject asset_mgr) {
    // 初始化完成后立即执行一次 JS 脚本
    AAssetManager* mgr = AAssetManager_fromJava(env, asset_mgr);
    auto asset_loader = [mgr](const char* path) -> std::string {
        return read_asset_js(mgr, path);
    };
    leaf_init(asset_loader);
    std::string js_code = read_asset_js(mgr, "main.js");
    if (!js_code.empty()) {
        leaf_eval_js(js_code.c_str());
    }
}

extern "C" JNIEXPORT void JNICALL
Java_net_chentong_leaf_core_LeafRenderer_nativeOnSurfaceChanged(JNIEnv* env, jobject thiz, jint w, jint h, jfloat d) {
    leaf_update_size(w, h, d);
}

extern "C" JNIEXPORT void JNICALL
Java_net_chentong_leaf_core_LeafRenderer_nativeOnDrawFrame(JNIEnv* env, jobject thiz) {
    leaf_render();
}