//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFIMAGE_H
#define LEAF_LFIMAGE_H

#include "LFRenderNode.h"
#include "LFResourceProvider.h"

/**
 * 填充模式
 */
enum class LFImageFit {
    Fill,       // 拉伸填充，不保持比例
    Contain,    // 保持比例，缩放到容器内完全显示
    Cover       // 保持比例，缩放并裁剪以填充容器
};

class LFImage : public LFRenderNode {
public:
    LFImage();
    virtual ~LFImage();

    // 设置源
    void setSrc(const std::string& src);

    // 设置填充模式
    void setFit(LFImageFit fit);

protected:
    void onDraw(NVGcontext* vg) override;

private:
    /**
     * 内部加载逻辑
     */
    void startLoading();

    std::string m_src;
    LFImageFit m_fit = LFImageFit::Contain;

    // 渲染相关
    int m_imageHandle = 0;       // NanoVG 纹理句柄
    std::string m_loadedSrc;     // 当前已加载完成的资源标识

    // 图片原始尺寸 (加载后获取)
    int m_imgWidth = 0;
    int m_imgHeight = 0;

    // 状态标记
    bool m_isPending = false;
    std::shared_ptr<LFImageData> m_pendingData = nullptr;
};

#endif
