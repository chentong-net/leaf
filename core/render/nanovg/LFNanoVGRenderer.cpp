#include "render/nanovg/LFNanoVGRenderer.h"

#include "LFDef.h"
#include "render/LFSceneRecorder.h"

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

struct LFNanoVGRenderer::FrameState {
    LFSceneRecorder currentScene;
    LFSceneRecorder previousScene;
    bool recording = false;
    bool sceneTargetReady = false;
    uint32_t pendingClearColor = 0;
    float frameWidth = 0.0f;
    float frameHeight = 0.0f;
    float pixelRatio = 1.0f;

#ifdef LEAF_USE_METAL
    MNVGframebuffer* sceneFramebuffer = nullptr;
#else
    unsigned int defaultFramebuffer = 0;
    unsigned int sceneFramebuffer = 0;
    unsigned int sceneTexture = 0;
    unsigned int sceneStencilBuffer = 0;
#endif
    int viewportWidth = 0;
    int viewportHeight = 0;
    int sceneWidth = 0;
    int sceneHeight = 0;

    void releaseGraphicsResources() {
#ifdef LEAF_USE_METAL
        if (sceneFramebuffer) {
            mnvgBindFramebuffer(nullptr);
            mnvgDeleteFramebuffer(sceneFramebuffer);
            sceneFramebuffer = nullptr;
        }
#else
        if (sceneFramebuffer != 0) {
            glDeleteFramebuffers(1, &sceneFramebuffer);
            sceneFramebuffer = 0;
        }
        if (sceneTexture != 0) {
            glDeleteTextures(1, &sceneTexture);
            sceneTexture = 0;
        }
        if (sceneStencilBuffer != 0) {
            glDeleteRenderbuffers(1, &sceneStencilBuffer);
            sceneStencilBuffer = 0;
        }
#endif
        sceneWidth = 0;
        sceneHeight = 0;
    }
};

LFNanoVGRenderer::LFNanoVGRenderer(void* context)
        : m_context(context),
          m_frameState(std::make_unique<FrameState>()) {
}

LFNanoVGRenderer::~LFNanoVGRenderer() {
    if (!m_context) return;

#ifdef LEAF_USE_METAL
    mnvgWaitUntilCompleted(toContext(m_context));
    m_frameState->releaseGraphicsResources();
    nvgDeleteMTL(toContext(m_context));
#elif defined(LEAF_USE_GL3)
    m_frameState->releaseGraphicsResources();
    nvgDeleteGL3(toContext(m_context));
#else
    m_frameState->releaseGraphicsResources();
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

    const float safePixelRatio =
        std::isfinite(pixelRatio) && pixelRatio > 0.0f ? pixelRatio : 1.0f;
    const int framebufferWidth = std::max(
        1,
        static_cast<int>(std::lround(width * safePixelRatio))
    );
    const int framebufferHeight = std::max(
        1,
        static_cast<int>(std::lround(height * safePixelRatio))
    );

    m_frameState->frameWidth = width;
    m_frameState->frameHeight = height;
    m_frameState->pixelRatio = safePixelRatio;
#ifndef LEAF_USE_METAL
    GLint currentFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer);
    m_frameState->defaultFramebuffer = static_cast<unsigned int>(currentFramebuffer);
#endif
    m_frameState->viewportWidth = framebufferWidth;
    m_frameState->viewportHeight = framebufferHeight;
    m_frameState->currentScene.beginFrame(
        width,
        height,
        safePixelRatio,
        m_frameState->pendingClearColor
    );
    m_frameState->recording = true;
    m_frameState->sceneTargetReady =
        prepareSceneTarget(framebufferWidth, framebufferHeight);
#ifndef LEAF_USE_METAL
    if (!m_frameState->sceneTargetReady) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_frameState->defaultFramebuffer);
        glViewport(0, 0, framebufferWidth, framebufferHeight);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, m_frameState->sceneFramebuffer);
        glViewport(0, 0, m_frameState->sceneWidth, m_frameState->sceneHeight);
    }
#endif

    // 初始化布局阶段的 NanoVG 测量状态，当前帧的绘制命令仍由场景记录器收集。
    nvgBeginFrame(context, width, height, safePixelRatio);
    nvgCancelFrame(context);
}

void LFNanoVGRenderer::endFrame() {
    NVGcontext* context = toContext(m_context);
    if (!context) return;

    m_frameState->recording = false;
    if (!m_frameState->sceneTargetReady) {
#ifdef LEAF_USE_METAL
        mnvgBindFramebuffer(nullptr);
#endif
        clearScene(true, {});
        nvgBeginFrame(
            context,
            m_frameState->frameWidth,
            m_frameState->frameHeight,
            m_frameState->pixelRatio
        );
        m_frameState->currentScene.replay(
            *this,
            {0.0f, 0.0f, m_frameState->frameWidth, m_frameState->frameHeight}
        );
        nvgEndFrame(context);
        m_frameState->previousScene = LFSceneRecorder{};
        return;
    }

    const LFDamageRegion damage =
        m_frameState->currentScene.diff(m_frameState->previousScene);
    if (damage.rects.empty()) {
        std::swap(m_frameState->currentScene, m_frameState->previousScene);
        compositeScene();
        return;
    }

#ifdef LEAF_USE_METAL
    // A Metal pass uses one hardware scissor, so merge damage conservatively.
    LFDrawRect damageBounds = damage.rects.front();
    for (size_t index = 1; index < damage.rects.size(); ++index) {
        const LFDrawRect& rect = damage.rects[index];
        const float left = std::min(damageBounds.x, rect.x);
        const float top = std::min(damageBounds.y, rect.y);
        const float right = std::max(
            damageBounds.x + damageBounds.width,
            rect.x + rect.width
        );
        const float bottom = std::max(
            damageBounds.y + damageBounds.height,
            rect.y + rect.height
        );
        damageBounds = {
            left,
            top,
            right - left,
            bottom - top,
        };
    }

    mnvgBindFramebuffer(m_frameState->sceneFramebuffer);
    if (damage.full) {
        clearScene(true, {});
    } else {
        const float scale = m_frameState->pixelRatio;
        const int left = std::clamp(
            static_cast<int>(std::floor(damageBounds.x * scale)),
            0,
            m_frameState->sceneWidth
        );
        const int top = std::clamp(
            static_cast<int>(std::floor(damageBounds.y * scale)),
            0,
            m_frameState->sceneHeight
        );
        const int right = std::clamp(
            static_cast<int>(std::ceil((damageBounds.x + damageBounds.width) * scale)),
            left,
            m_frameState->sceneWidth
        );
        const int bottom = std::clamp(
            static_cast<int>(std::ceil((damageBounds.y + damageBounds.height) * scale)),
            top,
            m_frameState->sceneHeight
        );
        mnvgSetScissorRect(context, left, top, right - left, bottom - top);
    }

    nvgBeginFrame(
        context,
        m_frameState->frameWidth,
        m_frameState->frameHeight,
        m_frameState->pixelRatio
    );
    if (!damage.full) {
        clearScene(false, damageBounds);
    }
    m_frameState->currentScene.replay(
        *this,
        damage.full
            ? LFDrawRect{0.0f, 0.0f,
                         m_frameState->frameWidth, m_frameState->frameHeight}
            : damageBounds
    );
    nvgEndFrame(context);
    if (!damage.full) {
        mnvgClearScissorRect(context);
    }
    mnvgBindFramebuffer(nullptr);
#else
    if (damage.full) {
        clearScene(true, {});
        nvgBeginFrame(
            context,
            m_frameState->frameWidth,
            m_frameState->frameHeight,
            m_frameState->pixelRatio
        );
        m_frameState->currentScene.replay(
            *this,
            {0.0f, 0.0f, m_frameState->frameWidth, m_frameState->frameHeight}
        );
        nvgEndFrame(context);
    } else {
        for (const LFDrawRect& rect : damage.rects) {
            clearScene(false, rect);
            glViewport(0, 0, m_frameState->sceneWidth, m_frameState->sceneHeight);
            const float scale = m_frameState->pixelRatio;
            const int left = std::clamp(
                static_cast<int>(std::floor(rect.x * scale)),
                0,
                m_frameState->sceneWidth
            );
            const int top = std::clamp(
                static_cast<int>(std::floor(rect.y * scale)),
                0,
                m_frameState->sceneHeight
            );
            const int right = std::clamp(
                static_cast<int>(std::ceil((rect.x + rect.width) * scale)),
                left,
                m_frameState->sceneWidth
            );
            const int bottom = std::clamp(
                static_cast<int>(std::ceil((rect.y + rect.height) * scale)),
                top,
                m_frameState->sceneHeight
            );
            glEnable(GL_SCISSOR_TEST);
            glScissor(
                left,
                m_frameState->sceneHeight - bottom,
                right - left,
                bottom - top
            );
            nvgBeginFrame(
                context,
                m_frameState->frameWidth,
                m_frameState->frameHeight,
                m_frameState->pixelRatio
            );
            m_frameState->currentScene.replay(*this, rect);
            nvgEndFrame(context);
            glDisable(GL_SCISSOR_TEST);
        }
    }
#endif

    std::swap(m_frameState->currentScene, m_frameState->previousScene);
    compositeScene();
}

void LFNanoVGRenderer::clear(uint32_t color) {
    m_frameState->pendingClearColor = color;
}

bool LFNanoVGRenderer::supportsDeferredPresentation() const {
    return m_frameState->sceneTargetReady;
}

void LFNanoVGRenderer::presentLastFrame() {
    compositeScene();
}

bool LFNanoVGRenderer::prepareSceneTarget(int framebufferWidth, int framebufferHeight) {
#ifdef LEAF_USE_METAL
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return false;
    }
    if (m_frameState->sceneFramebuffer &&
        m_frameState->sceneWidth == framebufferWidth &&
        m_frameState->sceneHeight == framebufferHeight) {
        return true;
    }

    NVGcontext* context = toContext(m_context);
    mnvgWaitUntilCompleted(context);
    m_frameState->releaseGraphicsResources();
    m_frameState->sceneFramebuffer = mnvgCreateLayerFramebuffer(
        context,
        framebufferWidth,
        framebufferHeight,
        NVG_IMAGE_NEAREST
    );
    if (!m_frameState->sceneFramebuffer) {
        return false;
    }

    m_frameState->sceneWidth = framebufferWidth;
    m_frameState->sceneHeight = framebufferHeight;
    return true;
#else
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return false;
    }
    if (m_frameState->sceneFramebuffer != 0 &&
        m_frameState->sceneWidth == framebufferWidth &&
        m_frameState->sceneHeight == framebufferHeight) {
        return true;
    }

    m_frameState->releaseGraphicsResources();
    glGenTextures(1, &m_frameState->sceneTexture);
    glBindTexture(GL_TEXTURE_2D, m_frameState->sceneTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        framebufferWidth,
        framebufferHeight,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glGenRenderbuffers(1, &m_frameState->sceneStencilBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_frameState->sceneStencilBuffer);
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        framebufferWidth,
        framebufferHeight
    );

    glGenFramebuffers(1, &m_frameState->sceneFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameState->sceneFramebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        m_frameState->sceneTexture,
        0
    );
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        m_frameState->sceneStencilBuffer
    );

    const bool complete =
        glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameState->defaultFramebuffer);
    if (!complete) {
        m_frameState->releaseGraphicsResources();
        return false;
    }

    m_frameState->sceneWidth = framebufferWidth;
    m_frameState->sceneHeight = framebufferHeight;
    return true;
#endif
}

void LFNanoVGRenderer::clearScene(bool full, const LFDrawRect& damage) {
#ifndef LEAF_USE_METAL
    const uint32_t color = m_frameState->pendingClearColor;
    const float alpha = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
    const float red = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    const float green = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
    const float blue = static_cast<float>(color & 0xFF) / 255.0f;

    glDisable(GL_SCISSOR_TEST);
    glClearColor(red, green, blue, alpha);
    if (full) {
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        return;
    }

    const float scale = m_frameState->pixelRatio;
    const int height = m_frameState->sceneHeight > 0
        ? m_frameState->sceneHeight
        : m_frameState->viewportHeight;
    const int left = std::clamp(
        static_cast<int>(std::floor(damage.x * scale)),
        0,
        m_frameState->sceneWidth
    );
    const int top = std::clamp(
        static_cast<int>(std::floor(damage.y * scale)),
        0,
        height
    );
    const int right = std::clamp(
        static_cast<int>(std::ceil((damage.x + damage.width) * scale)),
        left,
        m_frameState->sceneWidth
    );
    const int bottom = std::clamp(
        static_cast<int>(std::ceil((damage.y + damage.height) * scale)),
        top,
        height
    );
    if (right <= left || bottom <= top) {
        return;
    }
    glEnable(GL_SCISSOR_TEST);
    glScissor(left, height - bottom, right - left, bottom - top);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
#else
    NVGcontext* context = toContext(m_context);
    if (!context) return;

    if (full) {
        mnvgClearWithColor(context, toColor(m_frameState->pendingClearColor));
        return;
    }
    if (damage.width <= 0.0f || damage.height <= 0.0f) {
        return;
    }

    nvgSave(context);
    nvgResetTransform(context);
    nvgShapeAntiAlias(context, 0);
    nvgGlobalCompositeOperation(context, NVG_COPY);
    nvgBeginPath(context);
    nvgRect(
        context,
        0.0f,
        0.0f,
        m_frameState->frameWidth,
        m_frameState->frameHeight
    );
    nvgFillColor(context, toColor(m_frameState->pendingClearColor));
    nvgFill(context);
    nvgRestore(context);
#endif
}

void LFNanoVGRenderer::compositeScene() {
#ifndef LEAF_USE_METAL
    if (!m_frameState->sceneTargetReady) {
        return;
    }
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_frameState->sceneFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_frameState->defaultFramebuffer);
    glBlitFramebuffer(
        0,
        0,
        m_frameState->sceneWidth,
        m_frameState->sceneHeight,
        0,
        0,
        m_frameState->viewportWidth,
        m_frameState->viewportHeight,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameState->defaultFramebuffer);
    glViewport(
        0,
        0,
        m_frameState->viewportWidth,
        m_frameState->viewportHeight
    );
#else
    if (!m_frameState->sceneTargetReady || !m_frameState->sceneFramebuffer) {
        return;
    }

    NVGcontext* context = toContext(m_context);
    mnvgBindFramebuffer(nullptr);
    mnvgClearScissorRect(context);
    nvgBeginFrame(
        context,
        m_frameState->frameWidth,
        m_frameState->frameHeight,
        m_frameState->pixelRatio
    );
    nvgShapeAntiAlias(context, 0);
    nvgGlobalCompositeOperation(context, NVG_COPY);
    const NVGpaint paint = nvgImagePattern(
        context,
        0.0f,
        0.0f,
        m_frameState->frameWidth,
        m_frameState->frameHeight,
        0.0f,
        m_frameState->sceneFramebuffer->image,
        1.0f
    );
    nvgBeginPath(context);
    nvgRect(
        context,
        0.0f,
        0.0f,
        m_frameState->frameWidth,
        m_frameState->frameHeight
    );
    nvgFillPaint(context, paint);
    nvgFill(context);
    nvgEndFrame(context);
#endif
}

void LFNanoVGRenderer::pushState() {
    if (m_frameState->recording) {
        m_frameState->currentScene.pushState();
        return;
    }
    nvgSave(toContext(m_context));
}

void LFNanoVGRenderer::popState() {
    if (m_frameState->recording) {
        m_frameState->currentScene.popState();
        return;
    }
    nvgRestore(toContext(m_context));
}

void LFNanoVGRenderer::translate(float x, float y) {
    if (m_frameState->recording) {
        m_frameState->currentScene.translate(x, y);
        return;
    }
    nvgTranslate(toContext(m_context), x, y);
}

void LFNanoVGRenderer::rotate(float degrees) {
    if (m_frameState->recording) {
        m_frameState->currentScene.rotate(degrees);
        return;
    }
    nvgRotate(toContext(m_context), nvgDegToRad(degrees));
}

void LFNanoVGRenderer::scale(float x, float y) {
    if (m_frameState->recording) {
        m_frameState->currentScene.scale(x, y);
        return;
    }
    nvgScale(toContext(m_context), x, y);
}

void LFNanoVGRenderer::setOpacity(float opacity) {
    if (m_frameState->recording) {
        m_frameState->currentScene.setOpacity(opacity);
        return;
    }
    nvgGlobalAlpha(toContext(m_context), opacity);
}

void LFNanoVGRenderer::clipRect(const LFDrawRect& rect) {
    if (m_frameState->recording) {
        m_frameState->currentScene.clipRect(rect);
        return;
    }
    nvgIntersectScissor(
        toContext(m_context),
        rect.x,
        rect.y,
        rect.width,
        rect.height
    );
}

void LFNanoVGRenderer::drawRect(const LFDrawRect& rect, uint32_t color) {
    if (m_frameState->recording) {
        m_frameState->currentScene.drawRect(rect, color);
        return;
    }
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

    if (m_frameState->recording) {
        m_frameState->currentScene.drawRoundedRect(rect, radius, color);
        return;
    }
    NVGcontext* context = toContext(m_context);
    nvgBeginPath(context);
    nvgRoundedRect(context, rect.x, rect.y, rect.width, rect.height, radius);
    nvgFillColor(context, toColor(color));
    nvgFill(context);
}

void LFNanoVGRenderer::strokeRect(const LFDrawRect& rect, uint32_t color, float width) {
    if (m_frameState->recording) {
        m_frameState->currentScene.strokeRect(rect, color, width);
        return;
    }
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

    if (m_frameState->recording) {
        m_frameState->currentScene.strokeRoundedRect(rect, radius, color, width);
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
    if (m_frameState->recording) {
        m_frameState->currentScene.drawLine(x1, y1, x2, y2, color, width, cap);
        return;
    }
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

    if (m_frameState->recording) {
        m_frameState->currentScene.drawText(text);
        return;
    }
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

    if (m_frameState->recording) {
        m_frameState->currentScene.drawImage(image);
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

    if (m_frameState->recording) {
        m_frameState->currentScene.drawShadow(shadow);
        return;
    }
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
