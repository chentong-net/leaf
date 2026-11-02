#define NANOVG_GLES3_IMPLEMENTATION

#include "LFEngine.h"
#include "LFAppLaunch.h"
#include "text/LFFont.h"

extern "C" {

void leaf_process_pending_input();

void leaf_shutdown() {
    LFEngine& engine = LFEngine::getInstance();
    NVGcontext* vg = engine.getNVGContext();
    if (!vg) {
        return;
    }

    engine.shutdown();
    nvgDeleteGLES3(vg);
}

void leaf_init(std::function<std::vector<unsigned char>(const char *path)> loader) {
    if (LFEngine::getInstance().getNVGContext()) {
        leaf_shutdown();
    }

    int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES;
    NVGcontext* vg = nvgCreateGLES3(flags);

    if (!vg) return;

    LFEngine::getInstance().init(vg);

    for (const LFBundledFont& font : kLFBundledFonts) {
        const std::string path = "fonts/" + std::string(font.fileName);
        std::vector<unsigned char> fontData = loader(path.c_str());
        if (!fontData.empty()) {
            unsigned char* fontDataCopy =
                    static_cast<unsigned char*>(malloc(fontData.size()));
            if (fontDataCopy) {
                memcpy(fontDataCopy, fontData.data(), fontData.size());
                nvgCreateFontMem(
                        vg,
                        font.familyName,
                        fontDataCopy,
                        static_cast<int>(fontData.size()),
                        1);
            }
        }
    }

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
    static auto lastFrameTime = std::chrono::steady_clock::now();
    static bool hasFrameTime = false;
    const auto frameTime = std::chrono::steady_clock::now();
    const float dt = hasFrameTime
                     ? std::clamp(
                             std::chrono::duration<float>(frameTime - lastFrameTime).count(),
                             0.0f,
                             0.1f)
                     : 1.0f / 60.0f;
    lastFrameTime = frameTime;
    hasFrameTime = true;

    leaf_process_pending_input();
    LFEngine::getInstance().update(dt > 0.0f ? dt : 1.0f / 60.0f);

    // Render frame
    LFEngine::getInstance().render();
}

void leaf_eval_js(const char *code) {
    LFEngine::getInstance().setRoot(createAppRoot());
}

}
