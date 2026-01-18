//
// Created by Chen Tong on 2026/1/18.
//

#include "LFImage.h"

LFImage::LFImage() {
    YGNodeSetMeasureFunc(getYGNode(), measure);
}

LFImage::~LFImage() {
    // 依然存在析构时无法释放 handle 的问题，这通常由上层管理，
    // 但至少我们解决了运行时频繁切换图片的泄漏问题。
    // TODO: 释放纹理
}

void LFImage::setSrc(const std::string& src) {
    if (m_src == src) return;
    m_src = src;

    // ID 自增，任何之前的加载回调回来后发现 ID 对不上都会被丢弃
    m_loadRequestId++;

    // 不要在这里 m_imageHandle = 0 !
    // 保留旧图片显示，直到新图片下载完成 (Seamless Swap)。
    // 不要重置宽高，防止布局在加载过程中跳变。

    if (m_src.empty()) {
        // 如果是清空图片
        m_pendingData = nullptr;
        m_needUpload = true; // 触发 onDrawContent 去删除纹理

        // 清空时，尺寸确实应该归零
        m_imgWidth = 0;
        m_imgHeight = 0;
        YGNodeStyleSetAspectRatio(getYGNode(), NAN); // 清除比例
        YGNodeMarkDirty(getYGNode());
        markDirty();
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

    LFResourceProvider::getInstance().fetchImageBuffer(m_src, [weakThis, currentId](std::shared_ptr<LFImageData> imageData) {
        auto self = weakThis.lock();
        // 检查 ID 是否匹配，不匹配说明 setSrc 又被调过了，当前结果已过时
        if (!self || self->m_loadRequestId != currentId) return;

        if (imageData) {
            self->m_pendingData = imageData;
            self->m_needUpload = true; // 标记需要上传

            self->m_imgWidth = imageData->width;
            self->m_imgHeight = imageData->height;

            if (self->m_imgHeight > 0) {
                float ratio = (float)self->m_imgWidth / (float)self->m_imgHeight;
                YGNodeStyleSetAspectRatio(self->getYGNode(), ratio);
            }

            YGNodeMarkDirty(self->getYGNode());
            self->markDirty();
        }
    });
}

YGSize LFImage::measure(YGNodeRef node, float width, YGMeasureMode widthMode,
                        float height, YGMeasureMode heightMode) {
    auto* baseNode = static_cast<LFNode*>(YGNodeGetContext(node));
    auto* imgNode = static_cast<LFImage*>(baseNode);

    if (imgNode->m_imgWidth == 0 || imgNode->m_imgHeight == 0) {
        return {0, 0};
    }

    float intrinsicW = (float)imgNode->m_imgWidth;
    float intrinsicH = (float)imgNode->m_imgHeight;
    YGSize result = {0, 0};

    if (widthMode == YGMeasureModeExactly) {
        result.width = width;
    } else if (widthMode == YGMeasureModeAtMost) {
        result.width = std::min(width, intrinsicW);
    } else {
        result.width = intrinsicW;
    }

    if (heightMode == YGMeasureModeExactly) {
        result.height = height;
    } else if (heightMode == YGMeasureModeAtMost) {
        result.height = std::min(height, intrinsicH);
    } else {
        result.height = intrinsicH;
    }

    return result;
}

void LFImage::onDrawContent(NVGcontext* vg) {
    // 1. 纹理生命周期管理 (上传/删除/替换)
    // 只有在渲染线程持有 vg 时，我们才能安全操作 GPU 资源
    if (m_needUpload) {
        // 无论是要换新图，还是单纯清空，只要 handle 存在，先删掉旧的
        if (m_imageHandle > 0) {
            nvgDeleteImage(vg, m_imageHandle);
            m_imageHandle = 0;
        }

        // 如果有新数据，创建新纹理
        if (m_pendingData && m_pendingData->data) {
            m_imageHandle = nvgCreateImageMem(vg, 0, m_pendingData->data, m_pendingData->size);
        }

        m_pendingData = nullptr; // 释放 CPU 内存
        m_needUpload = false;    // 处理完毕
    }

    if (m_imageHandle <= 0) return;

    // 2. 计算 Content Box
    float paddingL = YGNodeLayoutGetPadding(getYGNode(), YGEdgeLeft);
    float paddingT = YGNodeLayoutGetPadding(getYGNode(), YGEdgeTop);
    float borderL = YGNodeLayoutGetBorder(getYGNode(), YGEdgeLeft);
    float borderT = YGNodeLayoutGetBorder(getYGNode(), YGEdgeTop);

    float contentX = paddingL + borderL;
    float contentY = paddingT + borderT;

    float layoutW = getLayoutWidth();
    float layoutH = getLayoutHeight();

    float contentW = layoutW - contentX - YGNodeLayoutGetPadding(getYGNode(), YGEdgeRight) - YGNodeLayoutGetBorder(getYGNode(), YGEdgeRight);
    float contentH = layoutH - contentY - YGNodeLayoutGetPadding(getYGNode(), YGEdgeBottom) - YGNodeLayoutGetBorder(getYGNode(), YGEdgeBottom);

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
    NVGpaint imgPaint = nvgImagePattern(vg, finalX, finalY, drawW, drawH, 0.0f, m_imageHandle, 1.0f);

    float radius = getRadius();
    // 计算"内圆角"。
    // 几何上，如果外边框圆角是 R，边框宽是 W，那么内部内容的圆角应该是 R - W。
    // 如果不减去，图片圆角会看起来比边框圆角大，导致四角有空隙或不自然。
    // 这里取左边框宽度做近似计算
    float effectiveRadius = 0.0f;
    if (radius > 0) {
        effectiveRadius = std::max(0.0f, radius - borderL);
    }
    nvgBeginPath(vg);

    if (m_fit == LFImageFit::Cover) {
        // Cover 模式下，纹理是放大的，但我们需要将绘制限制在 contentBox 内
        if (effectiveRadius > 0) {
            // Cover 模式下，我们填充的是 ContentBox 区域
            // 如果有圆角，使用 nvgRoundedRect
            nvgRoundedRect(vg, contentX, contentY, contentW, contentH, effectiveRadius);
        } else {
            nvgRect(vg, contentX, contentY, contentW, contentH);
        }
    } else {
        // Contain 或其他模式，我们绘制的是计算出的图片实际矩形 (finalX/Y)
        // 注意：通常在 Contain 模式下，如果图片很小悬浮在中间，是否要圆角取决于你的设计需求。
        // 这里假设只要设置了圆角，图片本身就应该应用圆角。
        if (effectiveRadius > 0) {
            nvgRoundedRect(vg, finalX, finalY, drawW, drawH, effectiveRadius);
        } else {
            nvgRect(vg, finalX, finalY, drawW, drawH);
        }
    }

    nvgFillPaint(vg, imgPaint);
    nvgFill(vg);
}
