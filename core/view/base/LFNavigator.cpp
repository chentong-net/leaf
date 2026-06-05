//
// Created by Chen Tong on 2026/1/28.
//

#include "LFNavigator.h"
#include "LFGlobalAnimationManager.h"
#include "animation/LFAnimator.h"

LFNavigator::Ptr LFNavigator::create() {
    auto nav = std::make_shared<LFNavigator>();
    nav->initStyle();
    return nav;
}

LFNavigator::LFNavigator() {
}

LFNavigator::~LFNavigator() {
}

void LFNavigator::initStyle() {
    matchParentWidth();
    matchParentHeight();
    setBackgroundColor(0xFF000000);
    // 开启裁剪，防止页面在动画时飞出屏幕可见
    setMasksToBounds(true);
}

LFPage::Ptr LFNavigator::getCurrentPage() const {
    if (m_stack.empty()) return nullptr;
    return m_stack.back();
}

void LFNavigator::push(LFPage::Ptr page, bool animated) {
    if (!page) return;
    if (m_isTransitioning) return;
    if (!m_stack.empty() && m_stack.back() == page) return;

    page->setNavigator(std::static_pointer_cast<LFNavigator>(shared_from_this()));

    LFPage::Ptr oldPage = nullptr;
    if (!m_stack.empty()) {
        oldPage = m_stack.back();
        oldPage->onDisappear();
    }

    // 利用 LFBox 的绝对定位堆叠特性
    // MatchParent 会自动设置 Left/Top/Right/Bottom = 0
    LFBox::addChild(page, LFBoxAlign::MatchParent);

    m_stack.push_back(page);
    page->onEnter();

    if (animated && oldPage) {
        page->setVisible(true);
        animatePush(oldPage, page);
    } else {
        if (oldPage) {
            oldPage->setVisible(false);
        }
        page->setVisible(true);
        page->setTranslate(0, 0);
        page->onAppear();
    }
}

void LFNavigator::pop(bool animated) {
    if (m_isTransitioning) return;
    if (m_stack.size() <= 1) return;

    auto exitPage = m_stack.back();
    auto enterPage = m_stack[m_stack.size() - 2];

    m_stack.pop_back();

    exitPage->onExit();

    if (animated) {
        animatePop(exitPage, enterPage);
    } else {
        removeChild(exitPage);
        // The previous page may have been hidden by an earlier animated push.
        enterPage->setVisible(true);
        enterPage->setTranslate(0, 0);
        enterPage->onAppear();
    }
}

void LFNavigator::replace(LFPage::Ptr page) {
    if (!page || m_isTransitioning) return;

    if (!m_stack.empty()) {
        auto oldPage = m_stack.back();
        oldPage->onDisappear();
        oldPage->onExit();
        removeChild(oldPage);
        m_stack.pop_back();
    }

    page->setNavigator(std::static_pointer_cast<LFNavigator>(shared_from_this()));
    LFBox::addChild(page, LFBoxAlign::MatchParent);
    m_stack.push_back(page);
    page->setVisible(true);
    page->setTranslate(0, 0);
    page->onEnter();
    page->onAppear();
}

// ==========================================
// 动画实现
// ==========================================

void LFNavigator::animatePush(LFPage::Ptr exitPage, LFPage::Ptr enterPage) {
    m_isTransitioning = true;

    float width = getLayoutWidth();
    if (width <= 0) width = 1080.0f; // 防御性默认值

    // 1. 初始状态
    enterPage->setTranslate(width, 0); // 新页面在右侧屏幕外
    exitPage->setTranslate(0, 0);      // 旧页面在原位

    // 2. 创建动画
    auto anim = LFValueAnimator<float>::of(0.0f, 1.0f);
    anim->setDuration(0.35f);
    // 使用 QuadOut (减速) 缓动
    anim->setEasing(LFEasingType::QuadOut);

    std::weak_ptr<LFPage> weakExit = exitPage;
    std::weak_ptr<LFPage> weakEnter = enterPage;
    std::weak_ptr<LFNavigator> weakSelf = std::static_pointer_cast<LFNavigator>(shared_from_this());

    anim->addUpdateListener([weakExit, weakEnter, width](const float& progress) {
        // 这里的 progress 已经经过 Easing 处理了 (如果 setEasing 生效)
        // 但为了保险或自定义视差，也可以用 raw fraction 自己算

        if (auto enter = weakEnter.lock()) {
            // 新页面：从 width -> 0
            float enterX = width * (1.0f - progress);
            enter->setTranslate(enterX, 0);
        }

        if (auto exit = weakExit.lock()) {
            // 旧页面：视差滚动，稍微左移 (0 -> -30%)
            float exitX = -(width * 0.3f) * progress;
            exit->setTranslate(exitX, 0);
        }
    });

    // [修正] 使用 setOnEnd 而不是 addCompletionListener
    anim->setOnEnd([weakSelf, weakExit, weakEnter]() {
        if (auto self = weakSelf.lock()) {
            self->m_isTransitioning = false;

            if (auto enter = weakEnter.lock()) {
                enter->setTranslate(0, 0); // 修正可能存在的浮点误差
                enter->onAppear();
            }
            if (auto exit = weakExit.lock()) {
                exit->setTranslate(0, 0); // 归位
                exit->setVisible(false);  // 隐藏不可见的旧页面以优化性能
            }
        }
    });

    anim->start();
    LFGlobalAnimationManager::getInstance().addAnimator(anim);
}

void LFNavigator::animatePop(LFPage::Ptr exitPage, LFPage::Ptr enterPage) {
    m_isTransitioning = true;

    float width = getLayoutWidth();

    // 1. 初始状态
    exitPage->setTranslate(0, 0);

    // 底层页面必须先设为可见，否则画不出来
    enterPage->setVisible(true);
    enterPage->setTranslate(-(width * 0.3f), 0); // 从视差位置开始

    // 2. 创建动画
    auto anim = LFValueAnimator<float>::of(0.0f, 1.0f);
    anim->setDuration(0.35f);
    anim->setEasing(LFEasingType::QuadOut);

    std::weak_ptr<LFPage> weakExit = exitPage;
    std::weak_ptr<LFPage> weakEnter = enterPage;
    std::weak_ptr<LFNavigator> weakSelf = std::static_pointer_cast<LFNavigator>(shared_from_this());

    anim->addUpdateListener([weakExit, weakEnter, width](const float& progress) {
        if (auto exit = weakExit.lock()) {
            // 当前页：向右滑出 (0 -> width)
            float exitX = width * progress;
            exit->setTranslate(exitX, 0);
        }

        if (auto enter = weakEnter.lock()) {
            // 底层页：复位 (-30% -> 0)
            float startX = -(width * 0.3f);
            float enterX = startX + (0.0f - startX) * progress;
            enter->setTranslate(enterX, 0);
        }
    });

    // [修正] 使用 setOnEnd
    anim->setOnEnd([weakSelf, weakExit, weakEnter]() {
        if (auto self = weakSelf.lock()) {
            self->m_isTransitioning = false;

            // 物理移除
            if (auto exit = weakExit.lock()) {
                self->removeChild(exit);
            }

            if (auto enter = weakEnter.lock()) {
                enter->setTranslate(0, 0);
                enter->onAppear();
            }
        }
    });

    anim->start();
    LFGlobalAnimationManager::getInstance().addAnimator(anim);
}
