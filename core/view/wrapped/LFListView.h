//
// Created by Chen Tong on 2026/2/14.
//

#ifndef LEAF_LFLISTVIEW_H
#define LEAF_LFLISTVIEW_H

#include "view/layout/LFBox.h"
#include "LFScrollView.h"
#include <map>
#include <unordered_map>

/**
 * 列表适配器接口
 * 用户实现该接口提供数据和Item视图
 */
class LFListAdapter {
public:
    using Ptr = std::shared_ptr<LFListAdapter>;
    virtual ~LFListAdapter() = default;

    /**
     * 获取数据总量
     */
    virtual int getCount() = 0;

    /**
     * 创建默认类型Item视图
     */
    virtual LFNode::Ptr createView() = 0;

    /**
     * 创建指定类型Item视图
     * 默认复用createView逻辑，子类可按需重写
     */
    virtual LFNode::Ptr createView(int viewType) {
        (void)viewType;
        return createView();
    }

    /**
     * 绑定指定索引的数据到Item视图
     */
    virtual void bindView(LFNode::Ptr view, int index) = 0;

    /**
     * 获取指定索引的Item类型
     * 默认只有一种类型
     */
    virtual int getItemViewType(int index) {
        (void)index;
        return 0;
    }

    /**
     * 获取指定索引的预估高度
     * 返回<=0表示使用LFListView的默认预估高度
     */
    virtual float getEstimatedItemExtent(int index) {
        (void)index;
        return -1.0f;
    }

    /**
     * 获取指定索引的Item高度
     * 返回<=0表示使用LFListView的默认高度
     */
    virtual float getItemExtent(int index) {
        (void)index;
        return -1.0f;
    }
};

/**
 * 垂直虚拟列表
 * 基于LFScrollView实现，支持可见区复用与按需渲染
 */
class LFListView : public LFBox {
public:
    using Ptr = std::shared_ptr<LFListView>;

    static Ptr createVertical();

    LFListView();
    virtual ~LFListView() = default;

    void setAdapter(LFListAdapter::Ptr adapter);
    LFListAdapter::Ptr getAdapter() const { return m_adapter; }

    /**
     * 数据变化后主动触发刷新
     */
    void notifyDataSetChanged();

    /**
     * 设置固定Item高度（V1为固定高度虚拟化方案）
     */
    void setItemExtent(float extent);
    float getItemExtent() const { return m_itemExtent; }

    /**
     * 设置可见区外预加载数量
     */
    void setPreloadCount(int count);
    int getPreloadCount() const { return m_preloadCount; }

    /**
     * 滚动到指定索引
     */
    void scrollToIndex(int index, bool animate = false);

    int getFirstVisibleIndex() const { return m_firstVisibleIndex; }
    int getLastVisibleIndex() const { return m_lastVisibleIndex; }

    float getScrollY() const;

    // 透传滚动容器配置
    void setScrollBarEnabled(bool enabled);
    bool isScrollBarEnabled() const;
    void setBounces(bool bounces);
    bool getBounces() const;

private:
    struct VisibleItem {
        LFNode::Ptr node;
        int viewType = 0;
    };

    void initLayout();
    void initFrameTask();

    void refreshVisibleWindow(bool forceRebind);
    void updateVisibleRange(int first, int last, bool forceRebind);
    void updateSpacerHeights(int first, int last);
    bool syncMeasuredItemExtents();
    void rebuildItemExtentCache(bool keepMeasured);
    void rebuildItemOffsets();
    float resolveItemExtent(int index) const;
    int findIndexByOffset(float offset) const;

    LFNode::Ptr obtainReusableItem(int viewType);
    void recycleItem(const VisibleItem& item);
    void clearVisibleItems(bool keepRecyclePool);
    void rebuildVisibleContainer(int first, int last);

    std::shared_ptr<LFScrollView> m_scrollView;
    std::shared_ptr<LFNode> m_topSpacer;
    std::shared_ptr<LFNode> m_bottomSpacer;
    std::shared_ptr<LFNode> m_visibleContainer;

    LFListAdapter::Ptr m_adapter = nullptr;
    std::map<int, VisibleItem> m_visibleItems;
    std::unordered_map<int, std::vector<LFNode::Ptr>> m_recyclePool;

    int m_itemCount = 0;
    int m_firstVisibleIndex = -1;
    int m_lastVisibleIndex = -1;

    float m_itemExtent = 72.0f;
    int m_preloadCount = 2;
    std::vector<float> m_itemExtents;
    std::vector<uint8_t> m_itemExtentStates; // 0=估算,1=固定,2=实测
    std::vector<float> m_itemOffsets;

    float m_lastScrollY = NAN;
    float m_lastViewportHeight = NAN;
    bool m_forceRefresh = true;
    bool m_frameTaskInstalled = false;
};

#endif // LEAF_LFLISTVIEW_H
