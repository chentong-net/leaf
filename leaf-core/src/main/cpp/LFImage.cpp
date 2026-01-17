//
// Created by Chen Tong on 2026/1/18.
//

#include "LFImage.h"

LFImage::LFImage() {
    // 初始状态下，图片节点通常不设置默认大小，由 Yoga 或外部样式决定
}

LFImage::~LFImage() {
    // 注意：纹理是由 NanoVG 创建的，如果 ResourceProvider 不做全局 handle 缓存，
    // 此处需要考虑是否由节点负责销毁。在本架构中建议 handle 随上下文管理。
}

void LFImage::setSrc(const std::string& src) {
    if (m_src == src) return;
    m_src = src;
    startLoading();
}

void LFImage::setFit(LFImageFit fit) {
    if (m_fit == fit) return;
    m_fit = fit;
    markDirty(); // 改变填充模式不需要重排布局，仅重绘
}

void LFImage::startLoading() {
    if (m_src.empty()) return;

    m_isPending = true;
    auto weakThis = std::weak_ptr<LFImage>(std::static_pointer_cast<LFImage>(shared_from_this()));

    // 通过 Provider 异步获取数据
    LFResourceProvider::getInstance().fetchImageBuffer(m_src, [this, weakThis](std::shared_ptr<LFImageData> imageData) {
        // 确保回调返回时节点依然存在
        if (imageData) {
            this->m_pendingData = imageData;
            this->markDirty(); // 标记需要重新绘制以触发纹理上传
        }
    });
}

void LFImage::onDraw(NVGcontext* vg) {
    // 1. 异步纹理上传逻辑 (仅执行一次)
    if (m_pendingData && m_pendingData->data) {
        // 在渲染线程调用 nvgCreateImageMem，因为它内部包含解码
        m_imageHandle = nvgCreateImageMem(vg, 0, m_pendingData->data, m_pendingData->size);

        if (m_imageHandle > 0) {
            nvgImageSize(vg, m_imageHandle, &m_imgWidth, &m_imgHeight);

            // 拿到尺寸后，如果之前没有设置固定宽高，可以更新 Yoga 布局
            // YGNodeStyleSetWidth(m_ygNode, (float)m_imgWidth);
            // YGNodeStyleSetHeight(m_ygNode, (float)m_imgHeight);
            // YGNodeMarkDirty(m_ygNode);
        }

        m_pendingData = nullptr; // 数据已上传 GPU，释放 CPU 内存
        m_isPending = false;
    }

    if (m_imageHandle <= 0) return;

    float viewW = getLayoutWidth();
    float viewH = getLayoutHeight();
    if (viewW <= 0 || viewH <= 0) return;

    // 2. 填充模式计算 (Object-Fit 逻辑)
    float drawX = 0, drawY = 0, drawW = viewW, drawH = viewH;
    float imgRatio = (float)m_imgWidth / (float)m_imgHeight;
    float viewRatio = viewW / viewH;

    if (m_fit == LFImageFit::Contain) {
        if (imgRatio > viewRatio) {
            drawW = viewW;
            drawH = viewW / imgRatio;
            drawY = (viewH - drawH) * 0.5f;
        } else {
            drawH = viewH;
            drawW = viewH * imgRatio;
            drawX = (viewW - drawW) * 0.5f;
        }
    } else if (m_fit == LFImageFit::Cover) {
        if (imgRatio > viewRatio) {
            drawH = viewH;
            drawW = viewH * imgRatio;
            drawX = (viewW - drawW) * 0.5f;
        } else {
            drawW = viewW;
            drawH = viewW / imgRatio;
            drawY = (viewH - drawH) * 0.5f;
        }
    }

    // 3. 渲染绘制
    // 使用 nvgImagePattern 配合变换实现缩放
    NVGpaint imgPaint = nvgImagePattern(vg, drawX, drawY, drawW, drawH, 0.0f, m_imageHandle, 1.0f);
    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, viewW, viewH); // 裁切区域即为容器大小
    nvgFillPaint(vg, imgPaint);
    nvgFill(vg);
}
