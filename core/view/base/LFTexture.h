//
// Created by Codex on 2026/2/26.
//

#ifndef LEAF_LFTEXTURE_H
#define LEAF_LFTEXTURE_H

#include "view/base/LFNode.h"

enum class LFTextureFit { Fill, Contain, Cover };

/**
 * 共享纹理组件
 * 1. 组件内部创建 GL 纹理
 * 2. 通过 textureId 暴露给第三方渲染
 * 3. Leaf 在 onDrawContent 中直接消费该纹理进行绘制
 */
class LFTexture : public LFNode {
public:
    using Ptr = std::shared_ptr<LFTexture>;
    using TextureCallback = std::function<void(GLuint textureId, int width, int height, uint64_t generation)>;

    LFTexture();
    ~LFTexture() override;

    static Ptr create();

    void setTextureSize(int width, int height);
    int getTextureWidth() const { return m_textureWidth; }
    int getTextureHeight() const { return m_textureHeight; }

    void setFit(LFTextureFit fit);
    LFTextureFit getFit() const { return m_fit; }

    GLuint getTextureId() const { return m_textureId; }
    uint64_t getTextureGeneration() const { return m_generation; }
    bool isTextureReady() const { return m_textureId != 0 && m_nvgImageHandle > 0; }

    // 第三方写入纹理后调用，通知引擎刷新
    void notifyFrameAvailable();

    // 主动请求重建纹理（上下文切换或外部资源重置时）
    void requestRecreate();

    void setOnTextureReady(TextureCallback callback);
    void setOnTextureWillRelease(TextureCallback callback);

protected:
    void onDrawContent(NVGcontext* vg) override;

private:
    void ensureTexture(NVGcontext* vg);
    void releaseTexture(NVGcontext* vg);

    int m_textureWidth = 0;
    int m_textureHeight = 0;
    int m_allocatedWidth = 0;
    int m_allocatedHeight = 0;
    LFTextureFit m_fit = LFTextureFit::Contain;

    GLuint m_textureId = 0;
    int m_nvgImageHandle = 0;
    uint64_t m_generation = 0;

    bool m_needRecreate = true;
    TextureCallback m_onTextureReady = nullptr;
    TextureCallback m_onTextureWillRelease = nullptr;
};

#endif // LEAF_LFTEXTURE_H
