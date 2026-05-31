//
// Created by Chen Tong on 2026/5/30.
// Base UI Component - Overlay Implementation
//

#include "LFOverlay.h"

#include "LFEngine.h"
#include "event/LFEvent.h"

LFOverlay::Ptr LFOverlay::create() {
    auto overlay = std::make_shared<LFOverlay>();
    return overlay;
}

LFOverlay::LFOverlay() {
    setPositionType(YGPositionTypeAbsolute);
    setPosition(YGEdgeAll, 0.0f);
    matchParentWidth();
    matchParentHeight();
    setTouchEnabled(false);
    setBackgroundColor(0x00000000);

    initLayout();
    updateBarrierState();
}

void LFOverlay::setModal(bool modal) {
    if (m_modal == modal) return;
    m_modal = modal;
    updateBarrierState();
}

void LFOverlay::setDismissOnBarrierTap(bool enabled) {
    if (m_dismissOnBarrierTap == enabled) return;
    m_dismissOnBarrierTap = enabled;
}

void LFOverlay::setBarrierColor(uint32_t color) {
    if (m_barrierColor == color) return;
    m_barrierColor = color;
    if (m_barrier) {
        m_barrier->setBackgroundColor(color);
    }
}

void LFOverlay::setContentOffset(float offsetX, float offsetY) {
    if (!m_contentLayer) return;
    m_contentLayer->setTranslate(offsetX, offsetY);
}

void LFOverlay::setOnDismiss(DismissCallback callback) {
    m_onDismiss = std::move(callback);
}

void LFOverlay::show(const LFNode::Ptr& content, LFBoxAlign align, float offsetX, float offsetY) {
    if (!content) {
        dismiss();
        return;
    }

    content->removeFromParent();
    clearActiveContent();
    m_contentLayer->addChild(content, align, offsetX, offsetY);
    m_activeContent = content;

    m_isShowing = true;
    ensureAttachedToRoot();
    updateBarrierState();
    setVisible(true);
}

void LFOverlay::dismiss() {
    // TODO: 目前没有detach from root，可能会内存泄漏
    if (!m_isShowing) {
        clearActiveContent();
        setVisible(false);
        return;
    }

    clearActiveContent();
    m_isShowing = false;
    setVisible(false);

    if (m_onDismiss) {
        m_onDismiss();
    }
}

void LFOverlay::initLayout() {
    m_barrier = LFBox::create();
    m_barrier->matchParentWidth();
    m_barrier->matchParentHeight();
    m_barrier->setBackgroundColor(m_barrierColor);
    m_barrier->setTouchEnabled(true);
    m_barrier->setOnTap([this](const LFPoint&) {
        if (!m_modal || !m_dismissOnBarrierTap) return;
        dismiss();
    });
    LFBox::addChild(m_barrier, LFBoxAlign::MatchParent);

    m_contentLayer = LFBox::create();
    m_contentLayer->matchParentWidth();
    m_contentLayer->matchParentHeight();
    m_contentLayer->setBackgroundColor(0x00000000);
    m_contentLayer->setTouchEnabled(false);
    LFBox::addChild(m_contentLayer, LFBoxAlign::MatchParent);
}

void LFOverlay::clearActiveContent() {
    if (m_activeContent) {
        m_activeContent->removeFromParent();
        m_activeContent.reset();
    }
}

void LFOverlay::ensureAttachedToRoot() {
    auto root = LFEngine::getInstance().getRoot();
    if (!root) return;

    if (getParent() != root.get()) {
        removeFromParent();
        root->addChild(shared_from_this());
    }
}

void LFOverlay::updateBarrierState() {
    if (!m_barrier) return;

    bool showBarrier = m_modal && m_isShowing;
    m_barrier->setVisible(showBarrier);
    m_barrier->setTouchEnabled(showBarrier);
    m_barrier->setBackgroundColor(showBarrier ? m_barrierColor : 0x00000000);
}
