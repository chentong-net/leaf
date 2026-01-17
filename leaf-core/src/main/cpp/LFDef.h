#include <GLES3/gl3.h>
extern "C" {
#include "quickjs.h"
}
#include "nanovg.h"
#include "nanovg_gl.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <functional>

// 内部日志宏，根据平台自动切换输出方式
#ifdef __ANDROID__
#include <android/log.h>
#define LF_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Leaf", __VA_ARGS__)
#else
#include <cstdio>
#define LF_LOGI(...) printf("[Leaf]: " __VA_ARGS__); printf("\n")
#endif

using PropValue = std::variant<float, uint32_t, std::string, double>;

using AssetLoaderFunc = std::function<std::string(const char* path)>;

// 图片缓存池
static std::unordered_map<std::string, int> g_image_cache;

// 定义通用渲染指令集
enum CommandType { CMD_RECT, CMD_CIRCLE, CMD_TEXT, CMD_CLEAR, CMD_IMAGE };

struct RenderCommand {
    CommandType type;
    std::unordered_map<std::string, PropValue> props;
};

// 引擎上下文结构体
struct EngineContext {
    NVGcontext* vg = nullptr;
    JSContext* ctx = nullptr;
    JSRuntime* rt = nullptr;

    // 帧回调
    JSValue frame_callback = JS_UNDEFINED;
    AssetLoaderFunc asset_loader = nullptr;

    // 待渲染指令队列
    std::vector<RenderCommand> command_queue;

    int width, height;
    float density;
};
