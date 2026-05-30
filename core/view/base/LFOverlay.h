//
// Created by Chen Tong on 2026/5/30.
// Base UI Component - Overlay
//

#ifndef LEAF_LFOVERLAY_H
#define LEAF_LFOVERLAY_H

#include "view/layout/LFBox.h"

/**
 * 基础弹出层容器
 *
 * 设计目标：
 * 1. 作为全局顶层承载层，覆盖普通布局树
 * 2. 支持模态遮罩与非模态浮层
 * 3. 提供内容插入与关闭能力
 *
 * 使用方式：
 * - 通过 show() 显示弹出内容，内部会自动挂到 root
 * - 通过 dismiss() 关闭
 */
class LFOverlay : public LFBox {
public:
    using Ptr = std::shared_ptr<LFOverlay>;
    using DismissCallback = std::function<void()>;

    static Ptr create();

    LFOverlay();
    ~LFOverlay() override = default;

    void setModal(bool modal);
    bool isModal() const { return m_modal; }

    void setDismissOnBarrierTap(bool enabled);
    bool isDismissOnBarrierTap() const { return m_dismissOnBarrierTap; }

    void setBarrierColor(uint32_t color);
    uint32_t getBarrierColor() const { return m_barrierColor; }

    void setContentOffset(float offsetX, float offsetY);

    void setOnDismiss(DismissCallback callback);

    void show(const LFNode::Ptr& content,
              LFBoxAlign align = LFBoxAlign::Center,
              float offsetX = 0.0f,
              float offsetY = 0.0f);
    void dismiss();

    bool isShowing() const { return m_isShowing; }
    LFNode::Ptr getActiveContent() const { return m_activeContent; }

private:
    void initLayout();
    void clearActiveContent();
    void updateBarrierState();
    void ensureAttachedToRoot();

    LFBox::Ptr m_barrier;
    LFBox::Ptr m_contentLayer;
    LFNode::Ptr m_activeContent;

    DismissCallback m_onDismiss;

    bool m_isShowing = false;
    bool m_modal = true;
    bool m_dismissOnBarrierTap = true;
    uint32_t m_barrierColor = 0x88000000;
};

#endif // LEAF_LFOVERLAY_H
