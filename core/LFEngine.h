//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFENGINE_H
#define LEAF_LFENGINE_H

#include "LFDef.h"
#include "view/base/LFNode.h"
#include "view/base/LFImage.h"
#include "view/base/LFText.h"
#include "view/base/LFInput.h"
#include "view/base/LFNavigator.h"
#include "view/base/LFPage.h"
#include "view/layout/LFLinear.h"
#include "view/layout/LFBox.h"
#include "view/layout/LFGrid.h"
#include "view/wrapped/LFScrollView.h"
#include "view/wrapped/LFPageView.h"
#include "view/wrapped/LFButton.h"
#include "view/wrapped/LFTab.h"
#include "event/LFEvent.h"
#include "event/LFEventDispatcher.h"
#include "event/LFGestureArena.h"
#include "event/LFHitTest.h"
#include "gesture/LFGestureRecognizer.h"
#include "animation/LFAnimator.h"
#include "LFGlobalAnimationManager.h"
#include "LFResourceProvider.h"
#include "LFJSONParser.h"
#include "LFState.h"

/**
 * 帧任务回调
 * 返回值：true 表示任务未完成，下一帧继续；false 表示任务结束，从队列移除。
 */
using LFFrameTask = std::function<bool()>;

/**
 * Leaf 引擎的核心驱动器
 */
class LFEngine {
public:
    // 单例访问
    static LFEngine &getInstance();

    // 禁止拷贝
    LFEngine(const LFEngine &) = delete;

    LFEngine &operator=(const LFEngine &) = delete;

    // 初始化与配置

    /**
     * 初始化引擎
     * @param vg NanoVG 上下文指针
     */
    void init(NVGcontext *vg);

    /**
     * 设置渲染窗口大小
     * 当窗口 resize 时必须调用
     */
    void setWindowSize(float width, float height, float scale);

    /**
     * 设置 UI 树的根节点
     */
    void setRoot(LFNode::Ptr root);

    LFNode::Ptr getRoot() const { return m_rootNode; }

    /**
     * 获取 NanoVG 上下文
     */
    NVGcontext *getNVGContext() const { return m_vg; }

    // --- 核心循环 (Game Loop) ---

    /**
     * 逻辑更新帧 (Update Tick)
     * 用于驱动动画/定时器/JS 逻辑等
     * @param dt Delta Time (秒)
     */
    void update(float dt);

    /**
     * 渲染帧 (Render Frame)
     * 包含：Layout 计算 -> 脏区清理 -> 绘制 -> 资源回收
     */
    void render();

    // 资源管理

    /**
     * 回收纹理 ID
     * 供 LFImage 析构时调用。将废弃的 handle 放入队列，
     * 引擎会在下一帧渲染结束时统一安全释放。
     */
    void recycleTexture(int imageHandle);

    /**
     * Get elapsed time since engine initialization (in seconds)
     * Used for gesture timing synchronization
     */
    double getElapsedTime() const;

    float getWindowWidth() const { return m_windowWidth; }
    float getWindowHeight() const { return m_windowHeight; }

    /**
     * 注册一个任务到主循环
     * 任务会在每帧的 update 阶段被调用执行一次
     */
    void addFrameTask(LFFrameTask task);

private:
    LFEngine() = default;

    ~LFEngine() = default;

    // 核心数据
    NVGcontext *m_vg = nullptr;
    LFNode::Ptr m_rootNode = nullptr;

    // 视口尺寸
    float m_windowWidth = 0.0f;
    float m_windowHeight = 0.0f;
    float m_pixelRatio = 1.0f;

    // 资源回收队列
    std::vector<int> m_garbageTextures;
    std::mutex m_gcMutex; // 保护回收队列，防止多线程析构冲突

    // Time base for gesture timing
    std::chrono::steady_clock::time_point m_startTime;

    // 任务队列
    std::vector<LFFrameTask> m_frameTasks;
    std::mutex m_taskMutex; // 保证任务注册的线程安全
};

#endif // LEAF_LFENGINE_H
