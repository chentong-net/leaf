//
// Created by Codex on 2026/2/26.
//

#include "view/base/LFTexture.h"
#include "LFEngine.h"

namespace {

float clampPositive(float value) {
    return std::max(0.0f, value);
}

}

LFTexture::LFTexture() = default;

LFTexture::~LFTexture() {
    // 析构可能不在渲染线程，使用引擎 GC 队列回收纹理资源。
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

void LFTexture::ensureTexture(LFRenderer* renderer) {
    if (!renderer) return;

    if (m_renderer && m_renderer != renderer) {
        m_nativeTextureId = 0;
        m_texture = 0;
        m_renderer = nullptr;
        m_allocatedWidth = 0;
        m_allocatedHeight = 0;
        m_needRecreate = true;
    }

    if (m_textureWidth <= 0 || m_textureHeight <= 0) {
        releaseTexture(renderer);
        return;
    }

    if (!m_needRecreate && m_texture != 0) return;

    releaseTexture(renderer);

    uintptr_t nativeTextureId = 0;
    const LFTextureHandle texture = renderer->createTexture(
        m_textureWidth,
        m_textureHeight,
        &nativeTextureId
    );
    if (texture == 0) {
        LF_LOGI("LFTexture: failed to create backend texture");
        return;
    }

    m_nativeTextureId = nativeTextureId;
    m_texture = texture;
    m_renderer = renderer;
    m_allocatedWidth = m_textureWidth;
    m_allocatedHeight = m_textureHeight;
    m_generation++;
    m_needRecreate = false;

    if (m_onTextureReady) {
        m_onTextureReady(m_nativeTextureId, m_allocatedWidth, m_allocatedHeight, m_generation);
    }
}

void LFTexture::releaseTexture(LFRenderer* renderer) {
    if (m_texture == 0 && m_nativeTextureId == 0) return;

    const uintptr_t oldTextureId = m_nativeTextureId;
    int oldWidth = m_allocatedWidth;
    int oldHeight = m_allocatedHeight;
    uint64_t oldGeneration = m_generation;

    if (m_onTextureWillRelease && oldTextureId != 0) {
        m_onTextureWillRelease(oldTextureId, oldWidth, oldHeight, oldGeneration);
    }

    if (m_texture > 0) {
        if (renderer && renderer == m_renderer) {
            renderer->releaseTexture(m_texture);
        } else {
            LFEngine::getInstance().recycleTexture(m_renderer, m_texture);
        }
    }

    m_nativeTextureId = 0;
    m_texture = 0;
    m_renderer = nullptr;
    m_allocatedWidth = 0;
    m_allocatedHeight = 0;
}

void LFTexture::onDrawContent(LFPaint& paint) {
    LFRenderer* renderer = LFEngine::getInstance().getRenderer();
    ensureTexture(renderer);
    if (m_texture == 0) return;
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
    float radius = getRadius();
    float effectiveRadius = radius > 0.0f ? std::max(0.0f, radius - borderL) : 0.0f;

    LFImageDraw imageDraw;
    imageDraw.texture = m_texture;
    imageDraw.radius = effectiveRadius;
    if (m_fit == LFTextureFit::Cover) {
        imageDraw.clipRect = {contentX, contentY, contentW, contentH};
    } else {
        imageDraw.clipRect = {finalX, finalY, drawW, drawH};
    }
    imageDraw.imageRect = {finalX, finalY, drawW, drawH};
    paint.drawImage(imageDraw);
}
