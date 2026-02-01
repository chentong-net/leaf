//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFIMAGE_H
#define LEAF_LFIMAGE_H

#include "LFNode.h"
#include "LFResourceProvider.h"

enum class LFImageFit { Fill, Contain, Cover };

/**
 * 图片组件
 */
class LFImage : public LFNode {
public:
    LFImage();
    virtual ~LFImage();

    void setSrc(const std::string& src); // 设置图片源
    void setFit(LFImageFit fit); // 设置填充模式

protected:
    void onDrawContent(NVGcontext* vg) override;

private:
    /**
     * 内部加载逻辑
     */
    void startLoading();
    static YGSize measure(YGNodeRef node, float width, YGMeasureMode widthMode,
                          float height, YGMeasureMode heightMode);

    std::string m_src;
    LFImageFit m_fit = LFImageFit::Contain;

    // 渲染状态
    int m_imageHandle = 0;
    int m_imgWidth = 0;
    int m_imgHeight = 0;

    // 异步加载状态
    std::shared_ptr<LFData> m_pendingData = nullptr;
    bool m_needUpload = false;

    // 加载请求 ID，解决异步竞态问题
    int m_loadRequestId = 0;
};

#endif // LEAF_LFIMAGE_H