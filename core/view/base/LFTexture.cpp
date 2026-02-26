//
// Created by Codex on 2026/2/26.
//

#include "view/base/LFTexture.h"
#include "LFEngine.h"

extern "C" {
#if defined(__DESKTOP__)
int nvglCreateImageFromHandleGL3(NVGcontext* ctx, GLuint textureId, int w, int h, int flags);
#else
int nvglCreateImageFromHandleGLES3(NVGcontext* ctx, GLuint textureId, int w, int h, int flags);
#endif
}

namespace {

float clampPositive(float value) {
    return std::max(0.0f, value);
}

}

LFTexture::LFTexture() = default;

LFTexture::~LFTexture() {
    // 析构可能不在渲染线程，使用引擎 GC 队列回收 NanoVG image handle。
    releaseTexture(nullptr);
}

LFTexture::Ptr LFTexture::create() {
    return std::make_shared<LFTexture>();
}

void LFTexture::setTextureSize(int width, int height) {
    int safeWidth = std::max(0, width);
    int safeHeight = std::max(0, height);
    if (m_textureWidth == safeWidth && m_textureHeight == safeHeight) return;

    m_textureWidth = safeWidth;
    m_textureHeight = safeHeight;
    m_needRecreate = true;
    markDirty();
}

void LFTexture::setFit(LFTextureFit fit) {
    if (m_fit == fit) return;
    m_fit = fit;
    markDirty();
}

void LFTexture::notifyFrameAvailable() {
    markDirty();
}

void LFTexture::requestRecreate() {
    m_needRecreate = true;
    markDirty();
}

void LFTexture::setOnTextureReady(TextureCallback callback) {
    m_onTextureReady = callback;
}

void LFTexture::setOnTextureWillRelease(TextureCallback callback) {
    m_onTextureWillRelease = callback;
}

void LFTexture::ensureTexture(NVGcontext* vg) {
    if (!vg) return;

    if (m_textureWidth <= 0 || m_textureHeight <= 0) {
        releaseTexture(vg);
        return;
    }

    if (!m_needRecreate && m_textureId != 0 && m_nvgImageHandle > 0) return;

    releaseTexture(vg);

    GLuint texture = 0;
    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_textureWidth, m_textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));

    if (texture == 0) {
        LF_LOGI("LFTexture: failed to create texture");
        return;
    }

    int imageHandle = 0;
#if defined(__DESKTOP__)
    imageHandle = nvglCreateImageFromHandleGL3(vg, texture, m_textureWidth, m_textureHeight, 0);
#else
    imageHandle = nvglCreateImageFromHandleGLES3(vg, texture, m_textureWidth, m_textureHeight, 0);
#endif

    if (imageHandle <= 0) {
        LF_LOGI("LFTexture: failed to create NanoVG image from texture");
        glDeleteTextures(1, &texture);
        return;
    }

    m_textureId = texture;
    m_nvgImageHandle = imageHandle;
    m_allocatedWidth = m_textureWidth;
    m_allocatedHeight = m_textureHeight;
    m_generation++;
    m_needRecreate = false;

    if (m_onTextureReady) {
        m_onTextureReady(m_textureId, m_allocatedWidth, m_allocatedHeight, m_generation);
    }
}

void LFTexture::releaseTexture(NVGcontext* vg) {
    if (m_textureId == 0 && m_nvgImageHandle <= 0) return;

    GLuint oldTextureId = m_textureId;
    int oldWidth = m_allocatedWidth;
    int oldHeight = m_allocatedHeight;
    uint64_t oldGeneration = m_generation;

    if (m_onTextureWillRelease && oldTextureId != 0) {
        m_onTextureWillRelease(oldTextureId, oldWidth, oldHeight, oldGeneration);
    }

    if (m_nvgImageHandle > 0) {
        if (vg) {
            nvgDeleteImage(vg, m_nvgImageHandle);
        } else {
            LFEngine::getInstance().recycleTexture(m_nvgImageHandle);
        }
    } else if (m_textureId != 0) {
        glDeleteTextures(1, &m_textureId);
    }

    m_textureId = 0;
    m_nvgImageHandle = 0;
    m_allocatedWidth = 0;
    m_allocatedHeight = 0;
}

void LFTexture::onDrawContent(NVGcontext* vg) {
    ensureTexture(vg);
    if (m_nvgImageHandle <= 0 || m_textureId == 0) return;
    if (m_allocatedWidth <= 0 || m_allocatedHeight <= 0) return;

    float paddingL = YGNodeLayoutGetPadding(getYGNode(), YGEdgeLeft);
    float paddingT = YGNodeLayoutGetPadding(getYGNode(), YGEdgeTop);
    float borderL = YGNodeLayoutGetBorder(getYGNode(), YGEdgeLeft);
    float borderT = YGNodeLayoutGetBorder(getYGNode(), YGEdgeTop);

    float contentX = paddingL + borderL;
    float contentY = paddingT + borderT;

    float layoutW = getLayoutWidth();
    float layoutH = getLayoutHeight();

    float contentW = layoutW - contentX
                     - YGNodeLayoutGetPadding(getYGNode(), YGEdgeRight)
                     - YGNodeLayoutGetBorder(getYGNode(), YGEdgeRight);
    float contentH = layoutH - contentY
                     - YGNodeLayoutGetPadding(getYGNode(), YGEdgeBottom)
                     - YGNodeLayoutGetBorder(getYGNode(), YGEdgeBottom);

    contentW = clampPositive(contentW);
    contentH = clampPositive(contentH);
    if (contentW <= 0.0f || contentH <= 0.0f) return;

    float drawX = 0.0f;
    float drawY = 0.0f;
    float drawW = contentW;
    float drawH = contentH;

    float texRatio = static_cast<float>(m_allocatedWidth) / static_cast<float>(m_allocatedHeight);
    float viewRatio = contentW / contentH;

    if (m_fit == LFTextureFit::Contain) {
        if (texRatio > viewRatio) {
            drawW = contentW;
            drawH = contentW / texRatio;
            drawY = (contentH - drawH) * 0.5f;
        } else {
            drawH = contentH;
            drawW = contentH * texRatio;
            drawX = (contentW - drawW) * 0.5f;
        }
    } else if (m_fit == LFTextureFit::Cover) {
        if (texRatio > viewRatio) {
            drawH = contentH;
            drawW = contentH * texRatio;
            drawX = (contentW - drawW) * 0.5f;
        } else {
            drawW = contentW;
            drawH = contentW / texRatio;
            drawY = (contentH - drawH) * 0.5f;
        }
    }

    float finalX = contentX + drawX;
    float finalY = contentY + drawY;
    NVGpaint paint = nvgImagePattern(vg, finalX, finalY, drawW, drawH, 0.0f, m_nvgImageHandle, 1.0f);

    float radius = getRadius();
    float effectiveRadius = radius > 0.0f ? std::max(0.0f, radius - borderL) : 0.0f;

    nvgBeginPath(vg);
    if (m_fit == LFTextureFit::Cover) {
        if (effectiveRadius > 0.0f) {
            nvgRoundedRect(vg, contentX, contentY, contentW, contentH, effectiveRadius);
        } else {
            nvgRect(vg, contentX, contentY, contentW, contentH);
        }
    } else {
        if (effectiveRadius > 0.0f) {
            nvgRoundedRect(vg, finalX, finalY, drawW, drawH, effectiveRadius);
        } else {
            nvgRect(vg, finalX, finalY, drawW, drawH);
        }
    }

    nvgFillPaint(vg, paint);
    nvgFill(vg);
}
