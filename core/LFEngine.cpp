//
// Created by 81153 on 2026/1/19.
//

#include "LFEngine.h"

#include <iterator>

LFEngine& LFEngine::getInstance() {
    static LFEngine instance;
    return instance;
}

void LFEngine::init(NVGcontext* vg) {
    if (m_vg && m_vg != vg) {
        discardContext();
    }

    m_vg = vg;

    // Initialize time base for gesture timing synchronization
    m_startTime = std::chrono::steady_clock::now();

    LFText::setMeasureContext(m_vg);
}

void LFEngine::shutdown() {
    NVGcontext* context = m_vg;
    LFEventDispatcher::getInstance().reset();
    LFGlobalAnimationManager::getInstance().cancelAll();
    m_rootNode.reset();

    std::vector<PendingTextureDeletion> garbageTextures;
    {
        std::lock_guard<std::mutex> lock(m_gcMutex);
        garbageTextures.swap(m_garbageTextures);
    }
    if (context) {
        for (const auto& pending : garbageTextures) {
            if (pending.context == context && pending.imageHandle > 0) {
                nvgDeleteImage(context, pending.imageHandle);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_frameTasks.clear();
    }

    m_textInputCursorCallback = nullptr;
    m_vg = nullptr;
    LFText::setMeasureContext(nullptr);
}

void LFEngine::discardContext() {
    LFEventDispatcher::getInstance().reset();
    LFGlobalAnimationManager::getInstance().cancelAll();
    m_rootNode.reset();

    {
        std::lock_guard<std::mutex> lock(m_gcMutex);
        m_garbageTextures.clear();
    }
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_frameTasks.clear();
    }

    m_textInputCursorCallback = nullptr;
    m_vg = nullptr;
    LFText::setMeasureContext(nullptr);
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
    LFGlobalAnimationManager::getInstance().update(dt);

    // 3. 处理帧任务。先交换出本帧快照，任务回调中可安全注册新任务。
    std::vector<LFFrameTask> frameTasks;
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        frameTasks.swap(m_frameTasks);
    }

    std::vector<LFFrameTask> pendingTasks;
    pendingTasks.reserve(frameTasks.size());
    for (auto& task : frameTasks) {
        if (!task) {
            continue;
        }

        bool keepAlive = false;
        try {
            keepAlive = task();
        } catch (...) {
            LF_LOGI("Frame task threw an exception and was removed");
        }

        if (keepAlive && task) {
            pendingTasks.push_back(std::move(task));
        }
    }

    if (!pendingTasks.empty()) {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_frameTasks.insert(
                m_frameTasks.begin(),
                std::make_move_iterator(pendingTasks.begin()),
                std::make_move_iterator(pendingTasks.end())
        );
    }

    // 3. 处理定时器 (TODO: LFTimer::update(dt))
    // 4. 执行 JS 里的 requestAnimationFrame
}


void LFEngine::render() {
    if (!m_vg || !m_rootNode) return;

#ifndef LEAF_USE_METAL
    int phyWidth = (int)(m_windowWidth * m_pixelRatio);
    int phyHeight = (int)(m_windowHeight * m_pixelRatio);
    glViewport(0, 0, phyWidth, phyHeight);
#endif

    // 先建立本帧的NanoVG上下文，确保文本测量和绘制使用相同的DPR
    nvgBeginFrame(m_vg, m_windowWidth, m_windowHeight, m_pixelRatio);

    // 布局
    // Yoga内部有缓存机制，如果根节点没有markDirty，calculateLayout几乎无消耗
    // 所以每帧调用是安全的，确保布局始终正确
    m_rootNode->calculateLayout(m_windowWidth, m_windowHeight);

    m_rootNode->render(m_vg);

    nvgEndFrame(m_vg);

    // 资源回收
    // 处理上一帧或本帧逻辑中死掉的组件遗留的纹理
    std::vector<PendingTextureDeletion> garbageTextures;
    {
        std::lock_guard<std::mutex> lock(m_gcMutex);
        garbageTextures.swap(m_garbageTextures);
    }

    for (const auto& pending : garbageTextures) {
        if (pending.context == m_vg && pending.imageHandle > 0) {
            nvgDeleteImage(m_vg, pending.imageHandle);
            LF_LOGI("GC Texture: %d", pending.imageHandle);
        }
    }
}

void LFEngine::recycleTexture(NVGcontext* context, int imageHandle) {
    if (!context || imageHandle <= 0) return;

    // 这是一个可能从任何线程调用的方法（取决于组件在哪析构）
    // 所以需要加锁保护
    std::lock_guard<std::mutex> lock(m_gcMutex);
    m_garbageTextures.push_back({context, imageHandle});
}

double LFEngine::getElapsedTime() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_startTime
    ).count() / 1000.0;  // Convert to seconds
}

void LFEngine::addFrameTask(LFFrameTask task) {
    if (!task) return;
    std::lock_guard<std::mutex> lock(m_taskMutex);
    m_frameTasks.push_back(task);
}

void LFEngine::setTextInputCursorCallback(LFTextInputCursorCallback callback) {
    m_textInputCursorCallback = callback;
}

void LFEngine::updateTextInputCursor(float x, float y, float lineHeight) {
    if (m_textInputCursorCallback) {
        m_textInputCursorCallback(x, y, lineHeight);
    }
}
