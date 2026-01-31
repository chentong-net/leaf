//
// Created by Chen Tong on 2026/1/31.
//

#ifndef LEAF_LFPAGEVIEW_H
#define LEAF_LFPAGEVIEW_H

#include "LFBox.h"
#include "animation/LFAnimator.h"
#include "gesture/LFGestureRecognizer.h"
#include <vector>
#include <functional>

/**
 * 页面适配器接口
 * 用户需实现此接口来提供数据和视图
 */
class LFPageAdapter {
public:
    using Ptr = std::shared_ptr<LFPageAdapter>;
    virtual ~LFPageAdapter() = default;

    /**
     * 获取总页数
     */
    virtual int getCount() = 0;

    /**
     * 创建一个页面视图
     * 引擎会复用此视图，整个 PageView 生命周期通常只创建 3 个实例
     */
    virtual LFNode::Ptr createView() = 0;

    /**
     * 绑定数据到视图
     * @param view 由 createView 创建的视图
     * @param index 数据索引 (0 ~ count-1)
     */
    virtual void bindView(LFNode::Ptr view, int index) = 0;
};

/**
 * 翻页视图容器 (Plan B: 平移连动式)
 *
 * 核心原理：
 * 维护 3 个常驻视图 (Prev, Curr, Next) 横向排列。
 * 滑动时同时移动这 3 个视图。
 * 翻页结束后，通过坐标归零和指针轮转 (Pointer Rotation) 实现无限滚动的障眼法。
 */
class LFPageView : public LFBox {
public:
    using Ptr = std::shared_ptr<LFPageView>;

    // 页面改变回调: (currentIndex)
    using OnPageChangedCallback = std::function<void(int)>;

    static Ptr create();

    LFPageView();
    virtual ~LFPageView();

    /**
     * 设置适配器 (核心数据源)
     * 设置后会自动刷新视图
     */
    void setAdapter(LFPageAdapter::Ptr adapter);
    LFPageAdapter::Ptr getAdapter() const { return m_adapter; }

    /**
     * 跳转到指定页
     * @param index 目标索引
     * @param animated 是否动画 (目前支持 false，未来可扩展动画跳转)
     */
    void setCurrentItem(int index, bool animated = false);

    int getCurrentItem() const { return m_currentIndex; }

    /**
     * 设置页面改变监听
     */
    void setOnPageChangeListener(OnPageChangedCallback callback);

private:
    void initLayout();
    void initGestures();

    // 核心渲染逻辑
    void refreshData();         // 数据源变动时完全重置
    void updateViewPositions(); // 根据 m_dragOffset 更新 3 个视图的位置
    void handlePanEnd(float velocityX); // 处理手势结束

    // 内部 helper
    void rotateViews(int direction); // 指针轮转：1(Next), -1(Prev)
    void bindPage(LFNode::Ptr node, int index); // 安全绑定 helper

    // 核心状态
    LFPageAdapter::Ptr m_adapter = nullptr;
    int m_currentIndex = 0;
    int m_totalCount = 0;

    // 视图容器 (复用池)
    // 逻辑上：0=Left/Prev, 1=Center/Curr, 2=Right/Next
    // 物理上：我们轮转这三个指针
    std::shared_ptr<LFNode> m_prevView;
    std::shared_ptr<LFNode> m_currView;
    std::shared_ptr<LFNode> m_nextView;

    // 交互状态
    float m_dragOffset = 0.0f; // 当前拖拽/动画偏移量
    float m_pageWidth = 0.0f;  // 缓存宽度

    // 动画
    std::shared_ptr<LFValueAnimator<float>> m_animator;
    OnPageChangedCallback m_onPageChange;
};

#endif //LEAF_LFPAGEVIEW_H
