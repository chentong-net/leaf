#ifndef LEAF_LFNANOVGRENDERER_H
#define LEAF_LFNANOVGRENDERER_H

#include "render/LFRenderer.h"

#include <memory>
#include <string>
#include <string_view>

/**
 * 使用 NanoVG 执行 Leaf 绘制操作的渲染后端。
 */
class LFNanoVGRenderer final : public LFRenderer {
public:
    static std::unique_ptr<LFRenderer> createOpenGL();
    static std::unique_ptr<LFRenderer> createMetal(void* nativeLayer);

    ~LFNanoVGRenderer() override;

    LFNanoVGRenderer(const LFNanoVGRenderer&) = delete;
    LFNanoVGRenderer& operator=(const LFNanoVGRenderer&) = delete;

    void beginFrame(float width, float height, float pixelRatio) override;
    void endFrame() override;
    void clear(uint32_t color) override;
    bool supportsDeferredPresentation() const override;
    void presentLastFrame() override;

    void pushState() override;
    void popState() override;
    void translate(float x, float y) override;
    void rotate(float degrees) override;
    void scale(float x, float y) override;
    void setOpacity(float opacity) override;
    void clipRect(const LFDrawRect& rect) override;

    void drawRect(const LFDrawRect& rect, uint32_t color) override;
    void drawRoundedRect(const LFDrawRect& rect, float radius, uint32_t color) override;
    void strokeRect(const LFDrawRect& rect, uint32_t color, float width) override;
    void strokeRoundedRect(const LFDrawRect& rect, float radius,
                           uint32_t color, float width) override;
    void drawLine(float x1, float y1, float x2, float y2,
                  uint32_t color, float width, LFStrokeCap cap) override;
    void drawText(const LFTextDraw& text) override;
    void drawImage(const LFImageDraw& image) override;
    void drawShadow(const LFShadowDraw& shadow) override;

    LFFontHandle createFontFromFile(const std::string& name,
                                    const std::string& path) override;
    LFFontHandle createFontFromMemory(const std::string& name,
                                      const void* data,
                                      size_t size,
                                      bool copyData) override;
    LFFontHandle findFont(std::string_view name) override;

    LFFontMetrics measureFontMetrics(LFFontHandle font,
                                     float fontSize,
                                     float lineHeight) override;
    LFTextBounds measureTextInkBounds(LFFontHandle font,
                                      float fontSize,
                                      float lineHeight,
                                      std::string_view text) override;
    LFTextBounds measureTextBounds(LFFontHandle font,
                                   float fontSize,
                                   LFTextOrigin origin,
                                   std::string_view text) override;

    LFTextureHandle createTextureFromMemory(const void* data, size_t size) override;
    LFTextureHandle createTexture(int width, int height,
                                  uintptr_t* nativeTexture) override;
    void releaseTexture(LFTextureHandle texture) override;

private:
    struct FrameState;

    explicit LFNanoVGRenderer(void* context);

    bool prepareSceneTarget(int framebufferWidth, int framebufferHeight);
    void clearScene(bool full, const LFDrawRect& damage);
    void compositeScene();

    void setTextState(LFFontHandle font, float fontSize, float lineHeight,
                      LFTextOrigin origin);

    void* m_context = nullptr;
    std::unique_ptr<FrameState> m_frameState;
};

#endif // LEAF_LFNANOVGRENDERER_H
