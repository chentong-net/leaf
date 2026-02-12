//
// Created by Chen Tong on 2026/1/31.
//

#include "LFPageView.h"
#include "LFGlobalAnimationManager.h"
#include "view/layout/LFLinear.h"

LFPageView::Ptr LFPageView::create() {
    auto view = std::make_shared<LFPageView>();
    view->initLayout(); // 初始化空容器
    view->initGestures();
    return view;
}

LFPageView::LFPageView() {
    // 默认撑满父容器
    matchParentWidth();
    matchParentHeight();
    // 关键：裁剪掉左右溢出的画面
    setMasksToBounds(true);
}

LFPageView::~LFPageView() {
    if (m_animator && m_animator->isRunning()) {
        m_animator->stop();
    }
}

void LFPageView::setAdapter(LFPageAdapter::Ptr adapter) {
    m_adapter = adapter;
    if (m_adapter) {
        m_totalCount = m_adapter->getCount();
    } else {
        m_totalCount = 0;
    }
    m_currentIndex = 0;

    // 彻底重置
    refreshData();
}

void LFPageView::setOnPageChangeListener(OnPageChangedCallback callback) {
    m_onPageChange = callback;
}

void LFPageView::setCurrentItem(int index, bool animated) {
    if (index < 0 || index >= m_totalCount) return;
    if (m_currentIndex == index) return;

    m_currentIndex = index;
    // 暂时只支持非动画跳转，因为跨页动画比较复杂
    refreshData();

    if (m_onPageChange) {
        m_onPageChange(m_currentIndex);
    }
}

void LFPageView::initLayout() {
    // 此时还没有 Adapter，我们先不创建具体的 View
    // 等到 refreshData 时再通过 adapter->createView() 创建
}

void LFPageView::refreshData() {
    // 清空现有子节点
    // 必须先物理移除旧节点，否则多次 refresh 会把旧页面叠在新页面上
    auto oldChildren = getChildren();
    for (const auto& child : oldChildren) {
        LFNode::removeChild(child);
    }

    m_prevView = nullptr;
    m_currView = nullptr;
    m_nextView = nullptr;

    if (!m_adapter) return;

    // Helper to create and setup wrapper
    auto setupView = [&](int slotIndex) -> LFNode::Ptr {
        auto node = m_adapter->createView();
        if (!node) {
            // 防御性编程：如果用户返回空，创建一个占位符
            node = LFBox::create();
        }
        // 强制绝对定位
        node->setPositionType(YGPositionTypeAbsolute);
        node->matchParentWidth();
        node->matchParentHeight();

        // 添加到自身
        this->addChild(node, LFBoxAlign::MatchParent);
        return node;
    };

    m_prevView = setupView(-1);
    m_currView = setupView(0);
    m_nextView = setupView(1);

    // 绑定初始数据
    bindPage(m_prevView, m_currentIndex - 1);
    bindPage(m_currView, m_currentIndex);
    bindPage(m_nextView, m_currentIndex + 1);

    m_dragOffset = 0;
    updateViewPositions();
}

void LFPageView::bindPage(LFNode::Ptr node, int index) {
    if (!node || !m_adapter) return;

    if (index >= 0 && index < m_totalCount) {
        node->setVisible(true);
        m_adapter->bindView(node, index);
    } else {
        // 越界（比如第0页的前一页），隐藏或显示为空
        node->setVisible(false);
    }
}

void LFPageView::notifyDataSetChanged() {
    if (!m_adapter) {
        m_totalCount = 0;
        return;
    }

    // 1. 获取最新的数据总量
    int newCount = m_adapter->getCount();
    if (newCount == m_totalCount) return; // 无变化则跳过

    m_totalCount = newCount;

    // 2. 核心：更新当前可见的三张页面的状态
    // 如果当前在最后一页，由于总数变多，原本隐藏的 m_nextView 需要被激活。

    // 刷新前一页可见性与内容
    bindPage(m_prevView, m_currentIndex - 1);

    // 刷新当前页内容
    bindPage(m_currView, m_currentIndex);

    // 刷新下一页：这是流式加载最关键的一步
    // 如果 currentIndex + 1 原本越界，现在不越界了，bindPage 内部会将其可见性设为 true
    bindPage(m_nextView, m_currentIndex + 1);

    // 3. 重新计算布局位置
    // 在 updateViewPositions 中，滑动阻尼限制会根据新的 m_totalCount 自动解除
    updateViewPositions();

    if (m_onPageChange) {
        m_onPageChange(m_currentIndex);
    }
}

void LFPageView::initGestures() {
    // 使用 Pan 手势
    setOnPan(
        // onUpdate
        [this](const LFPoint& delta, const LFPoint& velocity) {
            if (m_totalCount <= 0) return;

            m_dragOffset += delta.x;
            m_pageWidth = getLayoutWidth();

            // 边缘阻尼效果 (Rubber Banding)
            // 如果在第一页还往右拉 (offset > 0)，或者最后一页往左拉 (offset < 0)
            bool isFirst = m_currentIndex == 0;
            bool isLast = m_currentIndex == m_totalCount - 1;

            if ((isFirst && m_dragOffset > 0) || (isLast && m_dragOffset < 0)) {
                // 施加阻力：回退 delta 的大部分，只保留 40% 的移动量
                m_dragOffset -= delta.x * 0.6f;
            }

            updateViewPositions();
        },
        // onStart
        [this](const LFPoint& delta, const LFPoint& velocity) {
            if (m_totalCount <= 0) return;

            // 如果有正在进行的翻页/回弹动画，立即停止并接管
            if (m_animator && m_animator->isRunning()) {
                m_animator->stop();

                float threshold = m_pageWidth * 0.2f; // 阈值设小一点
                int pendingDirection = 0;

                if (m_dragOffset < -threshold) pendingDirection = 1;      // 去下一页
                else if (m_dragOffset > threshold) pendingDirection = -1; // 去上一页

                // 瞬间完成数据流转
                if (pendingDirection != 0) {
                     // 检查边界
                     if ((pendingDirection == 1 && m_currentIndex < m_totalCount - 1) ||
                         (pendingDirection == -1 && m_currentIndex > 0)) {
                         rotateViews(pendingDirection);
                     }
                }

                // 强制归零，准备接管新的拖拽
                // 因为 rotateViews 内部已经归零并刷新了 View，这里只需要确保万无一失
                m_dragOffset = 0;
                updateViewPositions();
            }
            m_pageWidth = getLayoutWidth();
        },
        // onEnd
        [this](const LFPoint& delta, const LFPoint& velocity) {
            if (m_totalCount <= 0) return;
            handlePanEnd(velocity.x);
        }
    );
}

void LFPageView::updateViewPositions() {
    if (m_pageWidth <= 0) m_pageWidth = getLayoutWidth();
    if (m_pageWidth <= 0) return; // 还没 Layout

    // Plan B 核心：三个页面连体移动
    // Prev:  -W + offset
    // Curr:   0 + offset
    // Next:  +W + offset

    if (m_prevView) m_prevView->setTranslate(-m_pageWidth + m_dragOffset, 0);
    if (m_currView) m_currView->setTranslate(m_dragOffset, 0);
    if (m_nextView) m_nextView->setTranslate(m_pageWidth + m_dragOffset, 0);
}

void LFPageView::handlePanEnd(float velocityX) {
    if (m_pageWidth <= 0) return;

    float threshold = m_pageWidth * 0.35f; // 35% 阈值
    float velocityThreshold = 800.0f;      // 快滑阈值

    float targetOffset = 0.0f;
    int direction = 0; // 0:Stay, 1:Next, -1:Prev

    // 1. 判断是否去下一页 (向左滑)
    // 条件：(位置超过阈值 OR 速度极快) AND 不是最后一页
    bool flingNext = velocityX < -velocityThreshold && m_dragOffset < 0;
    bool dragNext = m_dragOffset < -threshold;

    if ((flingNext || dragNext) && m_currentIndex < m_totalCount - 1) {
        targetOffset = -m_pageWidth;
        direction = 1;
    }
    // 2. 判断是否去上一页 (向右滑)
    else {
        bool flingPrev = velocityX > velocityThreshold && m_dragOffset > 0;
        bool dragPrev = m_dragOffset > threshold;

        if ((flingPrev || dragPrev) && m_currentIndex > 0) {
            targetOffset = m_pageWidth;
            direction = -1;
        }
    }

    // 3. 执行物理动画
    // 创建动画从当前 offset 到 target
    m_animator = LFValueAnimator<float>::of(m_dragOffset, targetOffset);

    // 使用 Spring 获得原生级手感
    // DampingRatio: 0.8 (较少震荡), Frequency: 0.5 (适中响应)
    m_animator->setSpring(0.8f, 0.5f);

    // 如果有初速度，传递给 Spring (需要 LFAnimator 支持 setStartVelocity，
    // 目前 LFSpringAdapter/LFValueAnimator 暂时没有暴露 velocity 接口，
    // 但 LFSpring 内部其实支持。这里暂时忽略初速度继承，直接用 Spring 逼近)
    // TODO: 未来在 LFValueAnimator 中添加 setStartVelocity 支持

    std::weak_ptr<LFPageView> weakSelf = std::static_pointer_cast<LFPageView>(shared_from_this());
    m_animator->addUpdateListener([weakSelf](float val) {
        if (auto self = weakSelf.lock()) {
            self->m_dragOffset = val;
            self->updateViewPositions();
        }
    });

    m_animator->setOnEnd([weakSelf, direction]() {
        if (auto self = weakSelf.lock()) {
            if (direction != 0) {
                // 只有真的翻页了才轮转
                self->rotateViews(direction);
            }
        }
    });

    m_animator->start();
    LFGlobalAnimationManager::getInstance().addAnimator(m_animator);
}

void LFPageView::rotateViews(int direction) {
    // 核心黑魔法：指针轮转 (Pointer Rotation)
    // 动画结束后，视觉上 target 页面已经位于屏幕中心 (offset = ±W)
    // 我们需要把 offset 瞬间归零，并调整指针，使得逻辑归位。

    if (direction == 1) {
        // === 向后翻 (Next -> Curr) ===
        // 视觉现状：Prev在-2W, Curr在-W, Next在0
        // 目标：Curr变成Prev, Next变成Curr, 原Prev变成Next放到最右边

        auto oldPrev = m_prevView;
        auto oldCurr = m_currView;
        auto oldNext = m_nextView;

        m_prevView = oldCurr; // 原来的 Curr 变成了现在的 Prev (逻辑上 index-1)
        m_currView = oldNext; // 原来的 Next 变成了现在的 Curr (逻辑上 index)
        m_nextView = oldPrev; // 原来的 Prev 被回收到最右边变成 Next

        m_currentIndex++;
    }
    else if (direction == -1) {
        // === 向前翻 (Prev -> Curr) ===
        // 视觉现状：Prev在0, Curr在W, Next在2W

        auto oldPrev = m_prevView;
        auto oldCurr = m_currView;
        auto oldNext = m_nextView;

        m_nextView = oldCurr; // 原来的 Curr 变成了现在的 Next
        m_currView = oldPrev; // 原来的 Prev 变成了现在的 Curr
        m_prevView = oldNext; // 原来的 Next 被回收到最左边变成 Prev

        m_currentIndex--;
    }

    // 1. 归零偏移
    m_dragOffset = 0;

    // 2. 重新绑定数据
    // 注意：因为我们轮转了指针，现在的 m_currView 其实就是刚才用户看到的那个 View
    // 它的内容已经是正确的了（因为之前做 Next/Prev 时绑定过）。
    // 所以只需要重新绑定那个“新回收过来”的 View 即可。

    if (direction == 1) {
        // 只需要刷新 m_nextView (新的 index + 1)
        bindPage(m_nextView, m_currentIndex + 1);
        // 为了安全，也可以重新 bind 其他的，但理论上不需要
    } else {
        // 只需要刷新 m_prevView (新的 index - 1)
        bindPage(m_prevView, m_currentIndex - 1);
    }

    // 3. 立即应用位置 (瞬间归位)
    // 因为 m_dragOffset 变回 0 了，且 m_currView 指针也换了
    // 视觉上用户完全无法察觉变化
    updateViewPositions();

    // 4. 回调
    if (m_onPageChange) {
        m_onPageChange(m_currentIndex);
    }
}
