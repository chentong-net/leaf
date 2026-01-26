//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFENGINE_H
#define LEAF_LFENGINE_H

#include "LFDef.h"
#include "LFNode.h"
#include "LFLinear.h"
#include "LFBox.h"
#include "LFText.h"
#include "LFImage.h"
#include "LFButton.h"
#include "LFScrollView.h"
#include "gesture/LFGestureRecognizer.h"
#include "LFResourceProvider.h"
#include "event/LFEvent.h"
#include "event/LFEventDispatcher.h"
#include "event/LFGestureArena.h"
#include "event/LFHitTest.h"
#include "animation/LFAnimator.h"
#include "LFGlobalAnimationManager.h"
#include "LFState.h"

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
};

#endif // LEAF_LFENGINE_H