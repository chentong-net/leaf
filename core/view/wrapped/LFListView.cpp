//
// Created by Chen Tong on 2026/2/14.
//

#include "LFListView.h"
#include "LFEngine.h"
#include "view/layout/LFLinear.h"

namespace {

int clampIndex(int value, int minValue, int maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

bool isValidExtent(float value) {
    return std::isfinite(value) && value > 0.0f;
}

}

LFListView::Ptr LFListView::createVertical() {
    auto view = std::make_shared<LFListView>();
    view->initFrameTask();
    return view;
}

LFListView::LFListView() {
    matchParentWidth();
    matchParentHeight();
    initLayout();
}

void LFListView::initLayout() {
    m_scrollView = LFScrollView::createVertical();
    m_scrollView->matchParentWidth();
    m_scrollView->matchParentHeight();

    m_topSpacer = LFBox::create();
    m_topSpacer->matchParentWidth();
    m_topSpacer->setHeight(0.0f);

    m_visibleContainer = LFLinear::createVertical();
    m_visibleContainer->matchParentWidth();
    m_visibleContainer->wrapContentHeight();

    m_bottomSpacer = LFBox::create();
    m_bottomSpacer->matchParentWidth();
    m_bottomSpacer->setHeight(0.0f);

    m_scrollView->addChild(m_topSpacer);
    m_scrollView->addChild(m_visibleContainer);
    m_scrollView->addChild(m_bottomSpacer);

    LFBox::addChild(m_scrollView, LFBoxAlign::MatchParent);
}

void LFListView::initFrameTask() {
    if (m_frameTaskInstalled) return;
    m_frameTaskInstalled = true;

    std::weak_ptr<LFListView> weakSelf = std::static_pointer_cast<LFListView>(shared_from_this());
    LFEngine::getInstance().addFrameTask([weakSelf]() {
        auto self = weakSelf.lock();
        if (!self) return false;
        self->refreshVisibleWindow(false);
        return true;
    });
}

void LFListView::setAdapter(LFListAdapter::Ptr adapter) {
    m_adapter = adapter;

    clearVisibleItems(false);
    m_recyclePool.clear();

    m_itemCount = m_adapter ? std::max(0, m_adapter->getCount()) : 0;
    rebuildItemExtentCache(false);

    if (m_scrollView) {
        m_scrollView->scrollTo(0.0f, false);
    }

    m_lastScrollY = NAN;
    m_lastViewportHeight = NAN;
    m_forceRefresh = true;
    refreshVisibleWindow(true);
}

void LFListView::notifyDataSetChanged() {
    if (!m_adapter) {
        m_itemCount = 0;
        m_itemExtents.clear();
        m_itemExtentStates.clear();
        m_itemOffsets.clear();
        clearVisibleItems(true);
        updateSpacerHeights(-1, -1);
        return;
    }

    m_itemCount = std::max(0, m_adapter->getCount());
    rebuildItemExtentCache(false);

    if (m_itemCount <= 0) {
        clearVisibleItems(true);
        updateSpacerHeights(-1, -1);
        return;
    }

    if (m_firstVisibleIndex >= m_itemCount) {
        scrollToIndex(m_itemCount - 1, false);
    }

    m_forceRefresh = true;
    m_lastScrollY = NAN;
    refreshVisibleWindow(true);
}

void LFListView::setItemExtent(float extent) {
    extent = std::max(1.0f, extent);
    if (std::fabs(m_itemExtent - extent) < 0.001f) return;

    m_itemExtent = extent;
    rebuildItemExtentCache(true);

    for (auto& entry : m_visibleItems) {
        int index = entry.first;
        auto node = entry.second.node;
        if (!node) continue;

        bool fixedExtent = index >= 0 && index < static_cast<int>(m_itemExtentStates.size()) &&
                           m_itemExtentStates[static_cast<size_t>(index)] == 1;
        if (fixedExtent) {
            node->setHeight(resolveItemExtent(index));
        } else {
            node->wrapContentHeight();
        }
    }

    m_forceRefresh = true;
    m_lastScrollY = NAN;
    refreshVisibleWindow(true);
}

void LFListView::setPreloadCount(int count) {
    count = std::max(0, count);
    if (m_preloadCount == count) return;

    m_preloadCount = count;
    m_forceRefresh = true;
    refreshVisibleWindow(true);
}

void LFListView::scrollToIndex(int index, bool animate) {
    if (!m_scrollView || m_itemCount <= 0) return;
    if (m_itemOffsets.size() != static_cast<size_t>(m_itemCount) + 1) {
        rebuildItemExtentCache(true);
    }

    int clamped = clampIndex(index, 0, m_itemCount - 1);
    float targetY = -m_itemOffsets[static_cast<size_t>(clamped)];
    m_scrollView->scrollTo(targetY, animate);

    m_forceRefresh = true;
    refreshVisibleWindow(true);
}

float LFListView::getScrollY() const {
    if (!m_scrollView) return 0.0f;
    return m_scrollView->getScrollY();
}

void LFListView::setScrollBarEnabled(bool enabled) {
    if (m_scrollView) {
        m_scrollView->setScrollBarEnabled(enabled);
    }
}

bool LFListView::isScrollBarEnabled() const {
    if (!m_scrollView) return false;
    return m_scrollView->isScrollBarEnabled();
}

void LFListView::setBounces(bool bounces) {
    if (m_scrollView) {
        m_scrollView->setBounces(bounces);
    }
}

bool LFListView::getBounces() const {
    if (!m_scrollView) return false;
    return m_scrollView->getBounces();
}

void LFListView::refreshVisibleWindow(bool forceRebind) {
    if (!m_adapter || m_itemCount <= 0 || !m_scrollView || !m_visibleContainer) {
        clearVisibleItems(true);
        updateSpacerHeights(-1, -1);
        return;
    }

    if (m_itemExtents.size() != static_cast<size_t>(m_itemCount) ||
        m_itemOffsets.size() != static_cast<size_t>(m_itemCount) + 1) {
        rebuildItemExtentCache(true);
    }

    // 先用上一帧layout结果回填实测高度，再进行可见区计算
    // 通过锚点补偿避免因为高度修正导致可视内容跳动
    syncMeasuredItemExtents();

    float viewportHeight = m_scrollView->getLayoutHeight();
    if (viewportHeight <= 0.0f) {
        return;
    }

    float scrollY = m_scrollView->getScrollY();
    bool scrollChanged = std::isnan(m_lastScrollY) || std::fabs(scrollY - m_lastScrollY) > 0.01f;
    bool viewportChanged = std::isnan(m_lastViewportHeight) || std::fabs(viewportHeight - m_lastViewportHeight) > 0.01f;

    if (!forceRebind && !m_forceRefresh && !scrollChanged && !viewportChanged) {
        return;
    }

    m_lastScrollY = scrollY;
    m_lastViewportHeight = viewportHeight;

    float visibleTop = std::max(0.0f, -scrollY);
    float visibleBottom = visibleTop + viewportHeight;

    int first = findIndexByOffset(visibleTop) - m_preloadCount;
    int last = findIndexByOffset(std::max(0.0f, visibleBottom - 0.1f)) + m_preloadCount;

    first = std::max(0, first);
    last = std::min(m_itemCount - 1, last);

    if (first > last) {
        clearVisibleItems(true);
        updateSpacerHeights(-1, -1);
        return;
    }

    bool rebindVisible = forceRebind || m_forceRefresh;
    if (!rebindVisible && first == m_firstVisibleIndex && last == m_lastVisibleIndex) {
        updateSpacerHeights(first, last);
        m_forceRefresh = false;
        return;
    }

    updateVisibleRange(first, last, rebindVisible);
    updateSpacerHeights(first, last);

    m_firstVisibleIndex = first;
    m_lastVisibleIndex = last;
    m_forceRefresh = false;
}

void LFListView::updateVisibleRange(int first, int last, bool forceRebind) {
    // 先回收窗口外Item
    for (auto it = m_visibleItems.begin(); it != m_visibleItems.end();) {
        int index = it->first;
        if (index < first || index > last) {
            recycleItem(it->second);
            it = m_visibleItems.erase(it);
        } else {
            ++it;
        }
    }

    // 补齐窗口内Item
    for (int index = first; index <= last; ++index) {
        int viewType = m_adapter->getItemViewType(index);

        auto it = m_visibleItems.find(index);
        bool needCreate = it == m_visibleItems.end();
        bool needBind = forceRebind || needCreate;
        LFNode::Ptr node;

        if (!needCreate) {
            node = it->second.node;
            bool typeChanged = it->second.viewType != viewType;
            if (typeChanged) {
                recycleItem(it->second);
                m_visibleItems.erase(it);
                node = nullptr;
                needCreate = true;
                needBind = true;
            }
        }

        if (needCreate) {
            node = obtainReusableItem(viewType);
            m_visibleItems[index] = {node, viewType};
        }

        if (!node) continue;

        if (node->getParent()) {
            node->removeFromParent();
        }

        // 复用节点可能遗留旧布局参数，进入列表前做统一归一
        node->setPositionType(YGPositionTypeRelative);
        node->setPosition(YGEdgeLeft, YGUndefined);
        node->setPosition(YGEdgeTop, YGUndefined);
        node->setPosition(YGEdgeRight, YGUndefined);
        node->setPosition(YGEdgeBottom, YGUndefined);
        node->setTranslate(0.0f, 0.0f);
        node->setTranslatePercent(0.0f, 0.0f);
        node->matchParentWidth();

        bool fixedExtent = index >= 0 && index < static_cast<int>(m_itemExtentStates.size()) &&
                           m_itemExtentStates[static_cast<size_t>(index)] == 1;
        if (fixedExtent) {
            node->setHeight(resolveItemExtent(index));
        } else {
            // 动态高度Item保持auto，让文本换行等真实布局自然测量
            node->wrapContentHeight();
        }

        if (needBind) {
            m_adapter->bindView(node, index);
        }
    }

    rebuildVisibleContainer(first, last);
}

void LFListView::updateSpacerHeights(int first, int last) {
    if (!m_topSpacer || !m_bottomSpacer) return;

    if (m_itemCount <= 0 || first < 0 || last < 0 || first > last ||
        m_itemOffsets.size() != static_cast<size_t>(m_itemCount) + 1) {
        m_topSpacer->setHeight(0.0f);
        m_bottomSpacer->setHeight(0.0f);
        return;
    }

    float topHeight = m_itemOffsets[static_cast<size_t>(first)];
    float bottomHeight = m_itemOffsets[static_cast<size_t>(m_itemCount)] -
                         m_itemOffsets[static_cast<size_t>(last) + 1];
    m_topSpacer->setHeight(std::max(0.0f, topHeight));
    m_bottomSpacer->setHeight(std::max(0.0f, bottomHeight));
}

bool LFListView::syncMeasuredItemExtents() {
    if (!m_adapter || !m_scrollView || m_visibleItems.empty()) {
        return false;
    }
    if (m_itemExtents.size() != static_cast<size_t>(m_itemCount) ||
        m_itemOffsets.size() != static_cast<size_t>(m_itemCount) + 1) {
        return false;
    }

    float scrollY = m_scrollView->getScrollY();
    float visibleTop = std::max(0.0f, -scrollY);
    int anchorIndex = findIndexByOffset(visibleTop);
    float anchorInnerOffset = 0.0f;
    if (anchorIndex >= 0) {
        anchorInnerOffset = visibleTop - m_itemOffsets[static_cast<size_t>(anchorIndex)];
    }

    bool changed = false;
    for (const auto& entry : m_visibleItems) {
        int index = entry.first;
        if (index < 0 || index >= m_itemCount) continue;

        if (m_itemExtentStates[static_cast<size_t>(index)] == 1) {
            continue;
        }

        auto node = entry.second.node;
        if (!node) continue;

        float measured = node->getLayoutHeight();
        if (!isValidExtent(measured)) {
            continue;
        }

        measured = std::max(1.0f, measured);
        float old = m_itemExtents[static_cast<size_t>(index)];
        if (std::fabs(measured - old) > 0.5f) {
            m_itemExtents[static_cast<size_t>(index)] = measured;
            m_itemExtentStates[static_cast<size_t>(index)] = 2;
            changed = true;
        }
    }

    if (!changed) {
        return false;
    }

    rebuildItemOffsets();

    if (anchorIndex >= 0 && anchorIndex < m_itemCount) {
        float targetVisibleTop = m_itemOffsets[static_cast<size_t>(anchorIndex)] + anchorInnerOffset;
        m_scrollView->scrollTo(-targetVisibleTop, false);
    }

    m_forceRefresh = true;
    m_lastScrollY = NAN;
    return true;
}

void LFListView::rebuildItemExtentCache(bool keepMeasured) {
    std::vector<float> oldExtents = std::move(m_itemExtents);
    std::vector<uint8_t> oldStates = std::move(m_itemExtentStates);

    m_itemExtents.assign(static_cast<size_t>(m_itemCount), std::max(1.0f, m_itemExtent));
    m_itemExtentStates.assign(static_cast<size_t>(m_itemCount), 0);

    bool canReuseMeasured = keepMeasured &&
                            oldExtents.size() == static_cast<size_t>(m_itemCount) &&
                            oldStates.size() == static_cast<size_t>(m_itemCount);

    for (int i = 0; i < m_itemCount; ++i) {
        float fixed = -1.0f;
        float estimated = -1.0f;

        if (m_adapter) {
            fixed = m_adapter->getItemExtent(i);
            estimated = m_adapter->getEstimatedItemExtent(i);
        }

        if (isValidExtent(fixed)) {
            m_itemExtents[static_cast<size_t>(i)] = std::max(1.0f, fixed);
            m_itemExtentStates[static_cast<size_t>(i)] = 1;
            continue;
        }

        if (canReuseMeasured && oldStates[static_cast<size_t>(i)] == 2 &&
            isValidExtent(oldExtents[static_cast<size_t>(i)])) {
            m_itemExtents[static_cast<size_t>(i)] = std::max(1.0f, oldExtents[static_cast<size_t>(i)]);
            m_itemExtentStates[static_cast<size_t>(i)] = 2;
            continue;
        }

        if (!isValidExtent(estimated)) {
            estimated = m_itemExtent;
        }

        m_itemExtents[static_cast<size_t>(i)] = std::max(1.0f, estimated);
        m_itemExtentStates[static_cast<size_t>(i)] = 0;
    }

    rebuildItemOffsets();
}

void LFListView::rebuildItemOffsets() {
    m_itemOffsets.assign(static_cast<size_t>(m_itemCount) + 1, 0.0f);
    for (int i = 0; i < m_itemCount; ++i) {
        float extent = resolveItemExtent(i);
        m_itemOffsets[static_cast<size_t>(i) + 1] = m_itemOffsets[static_cast<size_t>(i)] + extent;
    }
}

float LFListView::resolveItemExtent(int index) const {
    if (index < 0 || index >= m_itemCount) {
        return std::max(1.0f, m_itemExtent);
    }

    if (index < static_cast<int>(m_itemExtents.size()) &&
        isValidExtent(m_itemExtents[static_cast<size_t>(index)])) {
        return std::max(1.0f, m_itemExtents[static_cast<size_t>(index)]);
    }

    return std::max(1.0f, m_itemExtent);
}

int LFListView::findIndexByOffset(float offset) const {
    if (m_itemCount <= 0 || m_itemOffsets.empty()) {
        return -1;
    }

    if (offset <= 0.0f) {
        return 0;
    }

    float totalExtent = m_itemOffsets[static_cast<size_t>(m_itemCount)];
    if (offset >= totalExtent) {
        return m_itemCount - 1;
    }

    auto it = std::upper_bound(m_itemOffsets.begin(), m_itemOffsets.end(), offset);
    int index = static_cast<int>(it - m_itemOffsets.begin()) - 1;
    return clampIndex(index, 0, m_itemCount - 1);
}

LFNode::Ptr LFListView::obtainReusableItem(int viewType) {
    auto& pool = m_recyclePool[viewType];
    if (!pool.empty()) {
        auto node = pool.back();
        pool.pop_back();
        return node;
    }

    if (!m_adapter) return LFBox::create();

    auto node = m_adapter->createView(viewType);
    if (!node) {
        node = m_adapter->createView();
    }
    if (!node) {
        node = LFBox::create();
    }
    return node;
}

void LFListView::recycleItem(const VisibleItem& item) {
    if (!item.node) return;

    if (item.node->getParent()) {
        item.node->removeFromParent();
    }
    m_recyclePool[item.viewType].push_back(item.node);
}

void LFListView::clearVisibleItems(bool keepRecyclePool) {
    for (auto& entry : m_visibleItems) {
        if (keepRecyclePool) {
            recycleItem(entry.second);
        } else if (entry.second.node && entry.second.node->getParent()) {
            entry.second.node->removeFromParent();
        }
    }
    m_visibleItems.clear();

    if (!keepRecyclePool) {
        m_recyclePool.clear();
    }

    if (m_visibleContainer) {
        auto children = m_visibleContainer->getChildren();
        for (const auto& child : children) {
            m_visibleContainer->removeChild(child);
        }
    }

    m_firstVisibleIndex = -1;
    m_lastVisibleIndex = -1;
}

void LFListView::rebuildVisibleContainer(int first, int last) {
    if (!m_visibleContainer) return;

    auto children = m_visibleContainer->getChildren();
    for (const auto& child : children) {
        m_visibleContainer->removeChild(child);
    }

    for (int index = first; index <= last; ++index) {
        auto it = m_visibleItems.find(index);
        if (it == m_visibleItems.end()) continue;
        if (!it->second.node) continue;
        m_visibleContainer->addChild(it->second.node);
    }
}
