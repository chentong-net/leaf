#include "render/nanovg/LFNanoVGRenderer.h"

#include "LFDef.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

#include "nanovg.h"

#ifdef LEAF_USE_METAL
#include "nanovg_mtl.h"
#else
#if defined(LEAF_USE_GL3)
#define NANOVG_GL3_IMPLEMENTATION
#else
#define NANOVG_GLES3_IMPLEMENTATION
#endif
#include "nanovg_gl.h"
#endif

namespace {

NVGcontext* toContext(void* context) {
    return static_cast<NVGcontext*>(context);
}

NVGcolor toColor(uint32_t color) {
    return nvgRGBA(
        static_cast<unsigned char>((color >> 16) & 0xFF),
        static_cast<unsigned char>((color >> 8) & 0xFF),
        static_cast<unsigned char>(color & 0xFF),
        static_cast<unsigned char>((color >> 24) & 0xFF)
    );
}

LFFontHandle toFontHandle(int fontId) {
    return fontId >= 0 ? static_cast<LFFontHandle>(fontId) + 1 : 0;
}

int toFontId(LFFontHandle font) {
    return font > 0 ? static_cast<int>(font - 1) : -1;
}

LFTextureHandle toTextureHandle(int imageId) {
    return imageId > 0 ? static_cast<LFTextureHandle>(imageId) + 1 : 0;
}

int toImageId(LFTextureHandle texture) {
    return texture > 0 ? static_cast<int>(texture - 1) : 0;
}

int toNVGLineCap(LFStrokeCap cap) {
    switch (cap) {
        case LFStrokeCap::Round:
            return NVG_ROUND;
        case LFStrokeCap::Square:
            return NVG_SQUARE;
        case LFStrokeCap::Butt:
        default:
            return NVG_BUTT;
    }
}

} // namespace

LFNanoVGRenderer::LFNanoVGRenderer(void* context)
        : m_context(context) {
}

LFNanoVGRenderer::~LFNanoVGRenderer() {
    if (!m_context) return;

#ifdef LEAF_USE_METAL
    nvgDeleteMTL(toContext(m_context));
#elif defined(LEAF_USE_GL3)
    nvgDeleteGL3(toContext(m_context));
#else
    nvgDeleteGLES3(toContext(m_context));
#endif
}

std::unique_ptr<LFRenderer> LFNanoVGRenderer::createOpenGL() {
#ifdef LEAF_USE_METAL
    return nullptr;
#else
    const int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES;
#if defined(LEAF_USE_GL3)
    NVGcontext* context = nvgCreateGL3(flags);
#else
    NVGcontext* context = nvgCreateGLES3(flags);
#endif
    if (!context) {
        return nullptr;
    }
    return std::unique_ptr<LFRenderer>(new LFNanoVGRenderer(context));
#endif
}

std::unique_ptr<LFRenderer> LFNanoVGRenderer::createMetal(void* nativeLayer) {
#ifdef LEAF_USE_METAL
    if (!nativeLayer) {
        return nullptr;
    }
    const int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_TRIPLE_BUFFER;
    NVGcontext* context = nvgCreateMTL(nativeLayer, flags);
    if (!context) {
        return nullptr;
    }
    return std::unique_ptr<LFRenderer>(new LFNanoVGRenderer(context));
#else
    (void) nativeLayer;
    return nullptr;
#endif
}

void LFNanoVGRenderer::beginFrame(float width, float height, float pixelRatio) {
    NVGcontext* context = toContext(m_context);
    if (!context) return;

#ifndef LEAF_USE_METAL
    if (std::isfinite(width) && std::isfinite(height) &&
        std::isfinite(pixelRatio) && pixelRatio > 0.0f) {
        const int framebufferWidth = std::max(
            1,
            static_cast<int>(std::lround(width * pixelRatio))
        );
        const int framebufferHeight = std::max(
            1,
            static_cast<int>(std::lround(height * pixelRatio))
        );
        glViewport(0, 0, framebufferWidth, framebufferHeight);
    }
#endif

    nvgBeginFrame(context, width, height, pixelRatio);
}

void LFNanoVGRenderer::endFrame() {
    NVGcontext* context = toContext(m_context);
    if (!context) return;
    nvgEndFrame(context);
}

void LFNanoVGRenderer::clear(uint32_t color) {
#ifdef LEAF_USE_METAL
    NVGcontext* context = toContext(m_context);
    if (context) {
        mnvgClearWithColor(context, toColor(color));
    }
#else
    const float alpha = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
    const float red = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    const float green = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
    const float blue = static_cast<float>(color & 0xFF) / 255.0f;
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void LFNanoVGRenderer::pushState() {
    nvgSave(toContext(m_context));
}

void LFNanoVGRenderer::popState() {
    nvgRestore(toContext(m_context));
}

void LFNanoVGRenderer::translate(float x, float y) {
    nvgTranslate(toContext(m_context), x, y);
}

void LFNanoVGRenderer::rotate(float degrees) {
    nvgRotate(toContext(m_context), nvgDegToRad(degrees));
}

void LFNanoVGRenderer::scale(float x, float y) {
    nvgScale(toContext(m_context), x, y);
}

void LFNanoVGRenderer::setOpacity(float opacity) {
    nvgGlobalAlpha(toContext(m_context), opacity);
}

void LFNanoVGRenderer::clipRect(const LFDrawRect& rect) {
    nvgIntersectScissor(
        toContext(m_context),
        rect.x,
        rect.y,
        rect.width,
        rect.height
    );
}

void LFNanoVGRenderer::drawRect(const LFDrawRect& rect, uint32_t color) {
    NVGcontext* context = toContext(m_context);
    nvgBeginPath(context);
    nvgRect(context, rect.x, rect.y, rect.width, rect.height);
    nvgFillColor(context, toColor(color));
    nvgFill(context);
}

void LFNanoVGRenderer::drawRoundedRect(const LFDrawRect& rect, float radius, uint32_t color) {
    if (radius <= 0.0f) {
        drawRect(rect, color);
        return;
    }

    NVGcontext* context = toContext(m_context);
    nvgBeginPath(context);
    nvgRoundedRect(context, rect.x, rect.y, rect.width, rect.height, radius);
    nvgFillColor(context, toColor(color));
    nvgFill(context);
}

void LFNanoVGRenderer::strokeRect(const LFDrawRect& rect, uint32_t color, float width) {
    NVGcontext* context = toContext(m_context);
    nvgBeginPath(context);
    nvgRect(context, rect.x, rect.y, rect.width, rect.height);
    nvgStrokeColor(context, toColor(color));
    nvgStrokeWidth(context, width);
    nvgStroke(context);
}

void LFNanoVGRenderer::strokeRoundedRect(const LFDrawRect& rect, float radius,
                                         uint32_t color, float width) {
    if (radius <= 0.0f) {
        strokeRect(rect, color, width);
        return;
    }

    NVGcontext* context = toContext(m_context);
    nvgBeginPath(context);
    nvgRoundedRect(context, rect.x, rect.y, rect.width, rect.height, radius);
    nvgStrokeColor(context, toColor(color));
    nvgStrokeWidth(context, width);
    nvgStroke(context);
}

void LFNanoVGRenderer::drawLine(float x1, float y1, float x2, float y2,
                                uint32_t color, float width, LFStrokeCap cap) {
    NVGcontext* context = toContext(m_context);
    nvgBeginPath(context);
    nvgMoveTo(context, x1, y1);
    nvgLineTo(context, x2, y2);
    nvgStrokeColor(context, toColor(color));
    nvgStrokeWidth(context, width);
    nvgLineCap(context, toNVGLineCap(cap));
    nvgStroke(context);
}

void LFNanoVGRenderer::setTextState(LFFontHandle font, float fontSize,
                                    float lineHeight, LFTextOrigin origin) {
    NVGcontext* context = toContext(m_context);
    nvgFontFaceId(context, toFontId(font));
    nvgFontSize(context, fontSize);
    nvgTextLineHeight(context, lineHeight);
    nvgTextAlign(
        context,
        NVG_ALIGN_LEFT |
        (origin == LFTextOrigin::Top ? NVG_ALIGN_TOP : NVG_ALIGN_BASELINE)
    );
}

void LFNanoVGRenderer::drawText(const LFTextDraw& text) {
    if (text.font == 0 || text.text.empty()) return;

    NVGcontext* context = toContext(m_context);
    setTextState(text.font, text.fontSize, text.lineHeight, text.origin);
    nvgFillColor(context, toColor(text.color));

    if (text.italic) {
        nvgSave(context);
        nvgTranslate(context, text.x, text.y);
        nvgTransform(context, 1.0f, 0.0f, -0.18f, 1.0f, 0.0f, 0.0f);
        nvgText(context, 0.0f, 0.0f, text.text.data(), text.text.data() + text.text.size());
        nvgRestore(context);
    } else {
        nvgText(context, text.x, text.y,
                text.text.data(), text.text.data() + text.text.size());
    }
}

void LFNanoVGRenderer::drawImage(const LFImageDraw& image) {
    if (image.texture == 0 || image.imageRect.width <= 0.0f ||
        image.imageRect.height <= 0.0f || image.clipRect.width <= 0.0f ||
        image.clipRect.height <= 0.0f) {
        return;
    }

    NVGcontext* context = toContext(m_context);
    const NVGpaint paint = nvgImagePattern(
        context,
        image.imageRect.x,
        image.imageRect.y,
        image.imageRect.width,
        image.imageRect.height,
        0.0f,
        toImageId(image.texture),
        1.0f
    );

    nvgBeginPath(context);
    if (image.radius > 0.0f) {
        nvgRoundedRect(
            context,
            image.clipRect.x,
            image.clipRect.y,
            image.clipRect.width,
            image.clipRect.height,
            image.radius
        );
    } else {
        nvgRect(
            context,
            image.clipRect.x,
            image.clipRect.y,
            image.clipRect.width,
            image.clipRect.height
        );
    }
    nvgFillPaint(context, paint);
    nvgFill(context);
}

void LFNanoVGRenderer::drawShadow(const LFShadowDraw& shadow) {
    if (((shadow.color >> 24) & 0xFF) == 0) return;

    const float blur = shadow.blur;
    const float spread = shadow.spread;
    const float x = shadow.rect.x - spread;
    const float y = shadow.rect.y - spread;
    const float width = shadow.rect.width + spread * 2.0f;
    const float height = shadow.rect.height + spread * 2.0f;

    NVGcontext* context = toContext(m_context);
    const NVGpaint paint = nvgBoxGradient(
        context,
        shadow.offsetX,
        shadow.offsetY + 2.0f,
        shadow.rect.width,
        shadow.rect.height,
        shadow.radius,
        blur,
        toColor(shadow.color),
        nvgRGBA(0, 0, 0, 0)
    );

    nvgBeginPath(context);
    nvgRect(context, x - blur, y - blur,
            width + blur * 2.0f, height + blur * 2.0f);
    nvgRoundedRect(
        context,
        shadow.rect.x,
        shadow.rect.y,
        shadow.rect.width,
        shadow.rect.height,
        shadow.radius
    );
    nvgPathWinding(context, NVG_HOLE);
    nvgFillPaint(context, paint);
    nvgFill(context);
}

LFFontHandle LFNanoVGRenderer::createFontFromFile(const std::string& name,
                                                  const std::string& path) {
    return toFontHandle(nvgCreateFont(toContext(m_context), name.c_str(), path.c_str()));
}

LFFontHandle LFNanoVGRenderer::createFontFromMemory(const std::string& name,
                                                    const void* data,
                                                    size_t size,
                                                    bool copyData) {
    if (!data || size == 0 || size > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return 0;
    }

    unsigned char* fontData = nullptr;
    int freeData = 0;
    if (copyData) {
        fontData = static_cast<unsigned char*>(malloc(size));
        if (!fontData) return 0;
        memcpy(fontData, data, size);
        freeData = 1;
    } else {
        fontData = const_cast<unsigned char*>(static_cast<const unsigned char*>(data));
    }

    const int fontId = nvgCreateFontMem(
        toContext(m_context),
        name.c_str(),
        fontData,
        static_cast<int>(size),
        freeData
    );
    if (fontId < 0 && copyData) {
        free(fontData);
    }
    return toFontHandle(fontId);
}

LFFontHandle LFNanoVGRenderer::findFont(std::string_view name) {
    const std::string fontName(name);
    return toFontHandle(nvgFindFont(toContext(m_context), fontName.c_str()));
}

LFFontMetrics LFNanoVGRenderer::measureFontMetrics(LFFontHandle font,
                                                   float fontSize,
                                                   float lineHeight) {
    LFFontMetrics metrics;
    NVGcontext* context = toContext(m_context);
    if (!context || font == 0) return metrics;

    nvgSave(context);
    setTextState(font, fontSize, lineHeight, LFTextOrigin::Top);
    nvgTextMetrics(context, &metrics.ascender, &metrics.descender, &metrics.lineHeight);
    nvgRestore(context);
    return metrics;
}

LFTextBounds LFNanoVGRenderer::measureTextInkBounds(LFFontHandle font,
                                                    float fontSize,
                                                    float lineHeight,
                                                    std::string_view text) {
    LFTextBounds result;
    NVGcontext* context = toContext(m_context);
    if (!context || font == 0 || text.empty()) return result;

    float bounds[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    nvgSave(context);
    setTextState(font, fontSize, lineHeight, LFTextOrigin::Top);
    result.advance = nvgTextInkBounds(
        context,
        0.0f,
        0.0f,
        text.data(),
        text.data() + text.size(),
        bounds
    );
    result.minX = bounds[0];
    result.minY = bounds[1];
    result.maxX = bounds[2];
    result.maxY = bounds[3];
    nvgRestore(context);
    return result;
}

LFTextBounds LFNanoVGRenderer::measureTextBounds(LFFontHandle font,
                                                 float fontSize,
                                                 LFTextOrigin origin,
                                                 std::string_view text) {
    LFTextBounds result;
    NVGcontext* context = toContext(m_context);
    if (!context || font == 0 || text.empty()) return result;

    float bounds[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    nvgSave(context);
    setTextState(font, fontSize, 1.0f, origin);
    result.advance = nvgTextBounds(
        context,
        0.0f,
        0.0f,
        text.data(),
        text.data() + text.size(),
        bounds
    );
    result.minX = bounds[0];
    result.minY = bounds[1];
    result.maxX = bounds[2];
    result.maxY = bounds[3];
    nvgRestore(context);
    return result;
}

LFTextureHandle LFNanoVGRenderer::createTextureFromMemory(const void* data, size_t size) {
    if (!data || size == 0 || size > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return 0;
    }
    const int imageId = nvgCreateImageMem(
        toContext(m_context),
        0,
        const_cast<unsigned char*>(static_cast<const unsigned char*>(data)),
        static_cast<int>(size)
    );
    return toTextureHandle(imageId);
}

LFTextureHandle LFNanoVGRenderer::createTexture(int width, int height,
                                                uintptr_t* nativeTexture) {
    if (nativeTexture) {
        *nativeTexture = 0;
    }
    if (width <= 0 || height <= 0) return 0;

#ifdef LEAF_USE_METAL
    return 0;
#else
    GLuint texture = 0;
    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    if (texture == 0) return 0;

    int imageId = 0;
#if defined(LEAF_USE_GL3)
    imageId = nvglCreateImageFromHandleGL3(
        toContext(m_context), texture, width, height, 0);
#else
    imageId = nvglCreateImageFromHandleGLES3(
        toContext(m_context), texture, width, height, 0);
#endif
    if (imageId <= 0) {
        glDeleteTextures(1, &texture);
        return 0;
    }

    if (nativeTexture) {
        *nativeTexture = static_cast<uintptr_t>(texture);
    }
    return toTextureHandle(imageId);
#endif
}

void LFNanoVGRenderer::releaseTexture(LFTextureHandle texture) {
    const int imageId = toImageId(texture);
    if (imageId <= 0 || !m_context) return;
    nvgDeleteImage(toContext(m_context), imageId);
}
