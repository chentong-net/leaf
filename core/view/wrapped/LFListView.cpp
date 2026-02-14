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
        clearVisibleItems(true);
        updateSpacerHeights(-1, -1);
        return;
    }

    m_itemCount = std::max(0, m_adapter->getCount());

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

    for (auto& entry : m_visibleItems) {
        if (entry.second.node) {
            entry.second.node->setHeight(m_itemExtent);
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

    int clamped = clampIndex(index, 0, m_itemCount - 1);
    float targetY = -static_cast<float>(clamped) * m_itemExtent;
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

    int first = static_cast<int>(std::floor(visibleTop / m_itemExtent)) - m_preloadCount;
    int last = static_cast<int>(std::floor(std::max(0.0f, visibleBottom - 1.0f) / m_itemExtent)) + m_preloadCount;

    first = std::max(0, first);
    last = std::min(m_itemCount - 1, last);

    if (first > last) {
        clearVisibleItems(true);
        updateSpacerHeights(-1, -1);
        return;
    }

    bool rebindVisible = forceRebind || m_forceRefresh;
    if (!rebindVisible && first == m_firstVisibleIndex && last == m_lastVisibleIndex) {
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
        bool typeChanged = false;
        bool needBind = forceRebind || needCreate;
        LFNode::Ptr node;

        if (!needCreate) {
            node = it->second.node;
            typeChanged = it->second.viewType != viewType;
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
        node->setHeight(m_itemExtent);

        if (needBind) {
            m_adapter->bindView(node, index);
        }
    }

    rebuildVisibleContainer(first, last);
}

void LFListView::updateSpacerHeights(int first, int last) {
    if (!m_topSpacer || !m_bottomSpacer) return;

    if (m_itemCount <= 0 || first < 0 || last < 0 || first > last) {
        m_topSpacer->setHeight(0.0f);
        m_bottomSpacer->setHeight(0.0f);
        return;
    }

    float topHeight = static_cast<float>(first) * m_itemExtent;
    float bottomHeight = static_cast<float>(m_itemCount - last - 1) * m_itemExtent;
    m_topSpacer->setHeight(std::max(0.0f, topHeight));
    m_bottomSpacer->setHeight(std::max(0.0f, bottomHeight));
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
