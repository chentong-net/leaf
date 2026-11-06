//
// Created by Chen Tong on 2026/1/18.
//

#include "view/base/LFImage.h"
#include "LFEngine.h"
#include "plugin/LFPlugin.h"
#include "stb_image.h"

#include <limits>

LFImage::LFImage() {
    setLayoutMeasureFunction(
        [this](const LFMeasureInput& input) {
            return measureLayout(input);
        }
    );
}

LFImage::~LFImage() {
    if (m_texture > 0) {
        LFEngine::getInstance().recycleTexture(m_renderer, m_texture);
    }
}

void LFImage::setSrc(const std::string& src) {
    if (m_src == src) return;
    m_src = src;

    // ID 自增，任何之前的加载回调回来后发现 ID 对不上都会被丢弃
    m_loadRequestId++;

    // 不要在这里 m_texture = 0 !
    // 保留旧图片显示，直到新图片下载完成 (Seamless Swap)。
    // 不要重置宽高，防止布局在加载过程中跳变。

    if (m_src.empty()) {
        // 如果是清空图片
        m_pendingData = nullptr;
        m_needUpload = true; // 触发 onDrawContent 去删除纹理

        // 清空时，尺寸确实应该归零
        m_imgWidth = 0;
        m_imgHeight = 0;
        setAspectRatio(std::numeric_limits<float>::quiet_NaN());
        invalidateLayout();
    } else {
        startLoading();
    }
}

void LFImage::setFit(LFImageFit fit) {
    if (m_fit == fit) return;
    m_fit = fit;
    markDirty();
}

void LFImage::startLoading() {
    if (m_src.empty()) return;

    // 捕获当前的请求 ID
    int currentId = m_loadRequestId;
    auto weakThis = std::weak_ptr<LFImage>(std::static_pointer_cast<LFImage>(shared_from_this()));

    LFResourceProvider::getInstance().fetchAsset(m_src, [weakThis, currentId](std::shared_ptr<LFData> imageData) {
        LFPluginCenter::dispatchToMain(
                [weakThis, currentId, imageData = std::move(imageData)]() mutable {
                    auto self = weakThis.lock();
                    // 检查 ID 是否匹配，不匹配说明 setSrc 又被调过了，当前结果已过时
                    if (!self || self->m_loadRequestId != currentId || !imageData) {
                        return;
                    }

                    self->m_pendingData = std::move(imageData);
                    self->m_needUpload = true;

                    int width = 0;
                    int height = 0;
                    int channels = 0;
                    stbi_info_from_memory(
                            self->m_pendingData->data,
                            self->m_pendingData->size,
                            &width,
                            &height,
                            &channels
                    );
                    self->m_imgWidth = width;
                    self->m_imgHeight = height;
                    if (width > 0 && height > 0) {
                        self->setAspectRatio(
                            static_cast<float>(width) / static_cast<float>(height));
                    }

                    self->invalidateLayout();
                }
        );
    });
}

LFSize LFImage::measureLayout(const LFMeasureInput& input) const {
    if (m_imgWidth == 0 || m_imgHeight == 0) {
        return {};
    }

    float intrinsicW = (float)m_imgWidth;
    float intrinsicH = (float)m_imgHeight;
    LFSize result;

    if (input.width.mode == LFMeasureMode::Exactly) {
        result.width = input.width.size;
    } else if (input.width.mode == LFMeasureMode::AtMost) {
        result.width = std::min(input.width.size, intrinsicW);
    } else {
        result.width = intrinsicW;
    }

    if (input.height.mode == LFMeasureMode::Exactly) {
        result.height = input.height.size;
    } else if (input.height.mode == LFMeasureMode::AtMost) {
        result.height = std::min(input.height.size, intrinsicH);
    } else {
        result.height = intrinsicH;
    }

    return result;
}

void LFImage::onDrawContent(LFPaint& paint) {
    LFRenderer* renderer = LFEngine::getInstance().getRenderer();
    if (!renderer) return;

    if (m_texture > 0 && m_renderer != renderer) {
        m_texture = 0;
        m_renderer = nullptr;
        if (!m_src.empty()) {
            startLoading();
        }
    }

    // 1. 纹理生命周期管理 (上传/删除/替换)
    // 只有在渲染线程持有 Renderer 时，才能安全操作 GPU 资源
    if (m_needUpload) {
        // 无论是要换新图，还是单纯清空，只要 handle 存在，先删掉旧的
        if (m_texture > 0) {
            renderer->releaseTexture(m_texture);
            m_texture = 0;
            m_renderer = nullptr;
        }

        // 如果有新数据，创建新纹理
        if (m_pendingData && m_pendingData->data) {
            m_texture = renderer->createTextureFromMemory(
                m_pendingData->data,
                m_pendingData->size
            );
            if (m_texture > 0) {
                m_renderer = renderer;
                ++m_textureVersion;
            }
        }

        m_pendingData = nullptr; // 释放 CPU 内存
        m_needUpload = false;    // 处理完毕
    }

    if (m_texture == 0) return;

    // 2. 计算 Content Box
    const LFLayoutFrame& frame = getLayoutFrame();
    float contentX = frame.padding.left + frame.border.left;
    float contentY = frame.padding.top + frame.border.top;

    float layoutW = getLayoutWidth();
    float layoutH = getLayoutHeight();

    float contentW = layoutW - contentX - frame.padding.right - frame.border.right;
    float contentH = layoutH - contentY - frame.padding.bottom - frame.border.bottom;

    if (contentW <= 0 || contentH <= 0) return;

    // 3. Object-Fit 计算
    float drawX = 0, drawY = 0, drawW = contentW, drawH = contentH;
    float imgRatio = (float)m_imgWidth / (float)m_imgHeight;
    float viewRatio = contentW / contentH;

    if (m_fit == LFImageFit::Contain) {
        if (imgRatio > viewRatio) {
            drawW = contentW;
            drawH = contentW / imgRatio;
            drawY = (contentH - drawH) * 0.5f;
        } else {
            drawH = contentH;
            drawW = contentH * imgRatio;
            drawX = (contentW - drawW) * 0.5f;
        }
    } else if (m_fit == LFImageFit::Cover) {
        if (imgRatio > viewRatio) {
            drawH = contentH;
            drawW = contentH * imgRatio;
            drawX = (contentW - drawW) * 0.5f;
        } else {
            drawW = contentW;
            drawH = contentW / imgRatio;
            drawY = (contentH - drawH) * 0.5f;
        }
    }

    float finalX = contentX + drawX;
    float finalY = contentY + drawY;

    // 4. 绘制
    float radius = getRadius();
    // 计算"内圆角"。
    // 几何上，如果外边框圆角是 R，边框宽是 W，那么内部内容的圆角应该是 R - W。
    // 如果不减去，图片圆角会看起来比边框圆角大，导致四角有空隙或不自然。
    // 这里取左边框宽度做近似计算
    float effectiveRadius = 0.0f;
    if (radius > 0) {
        effectiveRadius = std::max(0.0f, radius - frame.border.left);
    }
    LFDrawRect clipRect;
    if (m_fit == LFImageFit::Cover) {
        clipRect = {contentX, contentY, contentW, contentH};
    } else {
        clipRect = {finalX, finalY, drawW, drawH};
    }

    LFImageDraw imageDraw;
    imageDraw.texture = m_texture;
    imageDraw.contentVersion = m_textureVersion;
    imageDraw.imageRect = {finalX, finalY, drawW, drawH};
    imageDraw.clipRect = clipRect;
    imageDraw.radius = effectiveRadius;
    paint.drawImage(imageDraw);
}
