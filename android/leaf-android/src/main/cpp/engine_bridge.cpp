#include "LFEngine.h"
#include "LFAppLaunch.h"
#include "render/nanovg/LFNanoVGRenderer.h"
#include "text/LFFont.h"

namespace {

std::unique_ptr<LFRenderer> g_renderer;

} // namespace

extern "C" {

void leaf_shutdown() {
    LFEngine& engine = LFEngine::getInstance();
    if (!engine.getRenderer()) {
        return;
    }

    engine.shutdown();
    g_renderer.reset();
}

void leaf_init(std::function<std::vector<unsigned char>(const char *path)> loader) {
    if (LFEngine::getInstance().getRenderer()) {
        leaf_shutdown();
    }

    g_renderer = LFNanoVGRenderer::createOpenGL();
    if (!g_renderer) return;

    // 2. 初始化引擎核心
    LFEngine::getInstance().init(*g_renderer);

    for (const LFBundledFont& font : kLFBundledFonts) {
        const std::string path = "fonts/" + std::string(font.fileName);
        std::vector<unsigned char> fontData = loader(path.c_str());
        if (!fontData.empty()) {
            g_renderer->createFontFromMemory(
                font.familyName,
                fontData.data(),
                fontData.size(),
                true
            );
        }
    }
    if (g_renderer->findFont(kLFDefaultFontFamily) == 0) {
        // Some custom ROMs do not expose the bundled font through the asset loader.
        g_renderer->createFontFromFile(
            kLFDefaultFontFamily,
            "/system/fonts/NotoSansCJK-Regular.ttc"
        );
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

    if (g_renderer) {
        g_renderer->clear(0xFF000000);
    }

    LFEngine::getInstance().update(dt > 0.0f ? dt : 1.0f / 60.0f);

    // Render frame
    LFEngine::getInstance().render();
}

void leaf_eval_js(const char *code) {
    LFEngine::getInstance().setRoot(createAppRoot());
}

}
