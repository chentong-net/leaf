#define NANOVG_GLES3_IMPLEMENTATION

#include "LFEngine.h"
#include "LFAppLaunch.h"

extern "C" {

void leaf_init(std::function<std::vector<unsigned char>(const char *path)> loader) {
    // 1. 创建 NanoVG 上下文
    int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES;
    NVGcontext* vg = nvgCreateGLES3(flags);

    if (!vg) return;

    // 2. 初始化引擎核心
    LFEngine::getInstance().init(vg);

    // 4. 加载字体
    std::vector<unsigned char> fontData = loader("fonts/Alibaba-PuHuiTi-Regular.ttf");
    unsigned char* fontDataCopy = (unsigned char*)malloc(fontData.size());
    memcpy(fontDataCopy, fontData.data(), fontData.size());
    if (nvgCreateFontMem(vg, "sans", fontDataCopy, fontData.size(), 1) == -1) {
        // If failed (e.g. some custom ROMs)
        nvgCreateFont(vg, "sans", "/system/fonts/NotoSansCJK-Regular.ttc");
    }

    // 3. 配置资源加载器
    LFResourceProvider::getInstance().setAssetLoader(
            [loader](const std::string& path, std::function<void(std::shared_ptr<LFData>)> callback) {
                std::vector<unsigned char> raw = loader(path.c_str());
                if (raw.empty()) {
                    callback(nullptr);
                    return;
                }

                auto data = std::make_shared<LFData>();
                data->size = raw.size();
                data->data = (unsigned char*)malloc(data->size);
                memcpy(data->data, raw.data(), data->size);

                callback(data);
            }
    );
}

void leaf_update_size(float w, float h, float d) {
    LFEngine::getInstance().setWindowSize((float)w, (float)h, d);
}

void leaf_render() {
    // Update logic (gesture timing, etc.)
    LFEngine::getInstance().update(0.016f);  // Assume 60fps (~16ms)

    // Render frame
    LFEngine::getInstance().render();
}

void leaf_eval_js(const char *code) {
    LFEngine::getInstance().setRoot(createAppRoot());
}

}
