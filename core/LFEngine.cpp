//
// Created by 81153 on 2026/1/19.
//

#include "LFEngine.h"
#include "event/LFEventDispatcher.h"
#include <chrono>

LFEngine& LFEngine::getInstance() {
    static LFEngine instance;
    return instance;
}

void LFEngine::init(NVGcontext* vg) {
    m_vg = vg;

    // Initialize time base for gesture timing synchronization
    m_startTime = std::chrono::steady_clock::now();

    LFText::setMeasureContext(m_vg);
    // 这里可以做一些 NanoVG 的全局配置，如加载默认字体等
    // nvgCreateFont(vg, "sans", "assets/Roboto-Regular.ttf");
}

void LFEngine::setWindowSize(float width, float height, float scale) {
    if (m_windowWidth == width && m_windowHeight == height && m_pixelRatio == scale) return;
    m_windowWidth = width;
    m_windowHeight = height;
    m_pixelRatio = scale;

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
    // 1. Update gesture recognizers (for LongPress timing)
    if (m_rootNode) {
        double currentTime = getElapsedTime();

        auto& dispatcher = LFEventDispatcher::getInstance();
        dispatcher.update(currentTime, m_rootNode);
    }

    // 2. 处理动画系统 (TODO: LFAnimator::update(dt))
    // 3. 处理定时器 (TODO: LFTimer::update(dt))
    // 4. 执行 JS 里的 requestAnimationFrame
}


void LFEngine::render() {
    if (!m_vg || !m_rootNode) return;

    int phyWidth = (int)(m_windowWidth * m_pixelRatio);
    int phyHeight = (int)(m_windowHeight * m_pixelRatio);
    glViewport(0, 0, phyWidth, phyHeight);

    // 布局
    // Yoga 内部有缓存机制，如果根节点没有 markDirty，calculateLayout 几乎无消耗
    // 所以每帧调用是安全的，确保布局始终正确
    m_rootNode->calculateLayout(m_windowWidth, m_windowHeight);

    // 绘制
    nvgBeginFrame(m_vg, m_windowWidth, m_windowHeight, m_pixelRatio);

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

double LFEngine::getElapsedTime() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_startTime
    ).count() / 1000.0;  // Convert to seconds
}
