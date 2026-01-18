//
// Created by 81153 on 2026/1/19.
//

#include "LFEngine.h"

LFEngine& LFEngine::getInstance() {
    static LFEngine instance;
    return instance;
}

void LFEngine::init(NVGcontext* vg) {
    m_vg = vg;
    // 这里可以做一些 NanoVG 的全局配置，如加载默认字体等
    // nvgCreateFont(vg, "sans", "assets/Roboto-Regular.ttf");
}

void LFEngine::setWindowSize(float width, float height) {
    if (m_windowWidth == width && m_windowHeight == height) return;
    m_windowWidth = width;
    m_windowHeight = height;

    // 窗口变了，根节点必须重新布局
    if (m_rootNode) {
        m_rootNode->markDirty();
    }
}

void LFEngine::setRoot(LFNode::Ptr root) {
    m_rootNode = root;
    if (m_rootNode) {
        m_rootNode->markDirty();
    }
}

void LFEngine::update(float dt) {
    // 1. 处理动画系统 (TODO: LFAnimator::update(dt))
    // 2. 处理定时器 (TODO: LFTimer::update(dt))
    // 3. 执行 JS 里的 requestAnimationFrame
}

void LFEngine::render() {
    if (!m_vg || !m_rootNode) return;

    // 布局
    // Yoga 内部有缓存机制，如果根节点没有 markDirty，calculateLayout 几乎无消耗
    // 所以每帧调用是安全的，确保布局始终正确
    m_rootNode->calculateLayout(m_windowWidth, m_windowHeight);

    // 绘制
    nvgBeginFrame(m_vg, m_windowWidth, m_windowHeight, 1.0f); // devicePixelRatio 暂定 1.0

    m_rootNode->render(m_vg);

    nvgEndFrame(m_vg);

    // 资源回收
    // 处理上一帧或本帧逻辑中死掉的组件遗留的纹理
    if (!m_garbageTextures.empty()) {
        std::lock_guard<std::mutex> lock(m_gcMutex);
        for (int handle : m_garbageTextures) {
            if (handle > 0) {
                nvgDeleteImage(m_vg, handle);
                LF_LOGI("GC Texture: %d", handle);
            }
        }
        m_garbageTextures.clear();
    }
}

void LFEngine::recycleTexture(int imageHandle) {
    if (imageHandle <= 0) return;

    // 这是一个可能从任何线程调用的方法（取决于组件在哪析构）
    // 所以需要加锁保护
    std::lock_guard<std::mutex> lock(m_gcMutex);
    m_garbageTextures.push_back(imageHandle);
}

void LFEngine::dispatchTouchEvent(LFTouchPhase phase, float x, float y) {
    if (!m_rootNode) return;

    // TODO: 实现 HitTest 算法
}
