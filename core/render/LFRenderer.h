#ifndef LEAF_LFRENDERER_H
#define LEAF_LFRENDERER_H

#include "render/LFPaint.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

/**
 * 渲染后端接口，负责帧生命周期以及字体和纹理资源。
 */
class LFRenderer : public LFPaint {
public:
    ~LFRenderer() override = default;

    virtual void beginFrame(float width, float height, float pixelRatio) = 0;
    virtual void endFrame() = 0;
    virtual void clear(uint32_t color) = 0;

    virtual LFFontHandle createFontFromFile(const std::string& name,
                                            const std::string& path) = 0;
    virtual LFFontHandle createFontFromMemory(const std::string& name,
                                              const void* data,
                                              size_t size,
                                              bool copyData) = 0;
    virtual LFFontHandle findFont(std::string_view name) = 0;

    virtual LFFontMetrics measureFontMetrics(LFFontHandle font,
                                             float fontSize,
                                             float lineHeight) = 0;
    virtual LFTextBounds measureTextInkBounds(LFFontHandle font,
                                              float fontSize,
                                              float lineHeight,
                                              std::string_view text) = 0;
    virtual LFTextBounds measureTextBounds(LFFontHandle font,
                                           float fontSize,
                                           LFTextOrigin origin,
                                           std::string_view text) = 0;

    virtual LFTextureHandle createTextureFromMemory(const void* data, size_t size) = 0;
    virtual LFTextureHandle createTexture(int width, int height,
                                          uintptr_t* nativeTexture) = 0;
    virtual void releaseTexture(LFTextureHandle texture) = 0;
};

#endif // LEAF_LFRENDERER_H
