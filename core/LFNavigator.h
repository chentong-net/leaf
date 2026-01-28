//
// Created by Chen Tong on 2026/1/29.
//

#ifndef LEAF_LFNAVIGATOR_H
#define LEAF_LFNAVIGATOR_H

#include "LFBox.h"
#include "LFPage.h"
#include <vector>

/**
 * 导航控制器 (Navigation Controller)
 *
 * 职责：
 * 1. 管理页面堆栈 (Page Stack).
 * 2. 处理页面跳转动画 (Push/Pop Transitions).
 * 3. 协调页面的生命周期调用.
 *
 * 架构：
 * 继承自 LFBox，利用其绝对定位 (Absolute Positioning) 能力来层叠页面。
 * 开启 setMasksToBounds(true) 以裁剪超出屏幕的页面内容。
 */
class LFNavigator : public LFBox {
public:
    using Ptr = std::shared_ptr<LFNavigator>;

    static Ptr create();

    LFNavigator();
    virtual ~LFNavigator();

    /**
     * 推入新页面 (Push)
     * @param page 要跳转的页面
     * @param animated 是否播放从右侧滑入的动画 (默认 true)
     */
    void push(LFPage::Ptr page, bool animated = true);

    /**
     * 退出当前页面 (Pop)
     * @param animated 是否播放滑出动画 (默认 true)
     */
    void pop(bool animated = true);

    /**
     * 获取栈顶页面 (当前显示的页面)
     */
    LFPage::Ptr getCurrentPage() const;

    /**
     * 获取当前堆栈深度
     */
    size_t getStackSize() const { return m_stack.size(); }

    /**
     * 替换当前页面 (Replace)
     * 将栈顶页面替换为新页面，不改变栈深度 (常用于登录成功后切换到主页)
     */
    void replace(LFPage::Ptr page);

private:
    void initStyle();

    // 页面堆栈，back() 为栈顶
    std::vector<LFPage::Ptr> m_stack;

    // 动画状态锁，防止转场过程中重复操作
    bool m_isTransitioning = false;

    // --- 动画实现 ---
    // Push: enterPage 从右侧进入，exitPage(当前页) 视差左移
    void animatePush(LFPage::Ptr exitPage, LFPage::Ptr enterPage);

    // Pop: exitPage 向右滑出，enterPage(下层页) 从视差位置复位
    void animatePop(LFPage::Ptr exitPage, LFPage::Ptr enterPage);
};

#endif // LEAF_LFNAVIGATOR_H
