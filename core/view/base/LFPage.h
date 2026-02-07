//
// Created by Chen Tong on 2026/1/29.
//

#ifndef LEAF_LFPAGE_H
#define LEAF_LFPAGE_H

#include "view/base/LFNode.h"

// 前置声明，避免循环依赖
class LFNavigator;

/**
 * 页面基类 (Base Page Class)
 *
 * 职责：
 * 1. 作为业务内容的顶级容器 (Root Container for content).
 * 2. 管理页面生命周期 (Lifecycle).
 * 3. 拦截底层点击事件，防止穿透.
 *
 * 继承关系：
 * LFPage -> LFNode
 */
class LFPage : public LFNode {
public:
    using Ptr = std::shared_ptr<LFPage>;

    /**
     * 工厂方法
     * 创建一个默认样式的页面（全屏、白底）
     */
    static Ptr create();

    LFPage();
    virtual ~LFPage() = default;

    // ==========================================
    // 生命周期回调 (Lifecycle Callbacks)
    // ==========================================

    /**
     * 当页面被推入导航栈时调用 (Push)
     * 此时页面可能还未显示（动画开始前）
     * 适合做数据初始化
     */
    virtual void onEnter() {}

    /**
     * 当页面即将从导航栈移除时调用 (Pop)
     * 此时页面还在屏幕上（动画开始前）
     * 适合做资源释放、取消网络请求
     */
    virtual void onExit() {}

    /**
     * 当页面完全可见时调用 (Did Appear)
     * 包括：Push 动画结束、或上层页面 Pop 后露出当前页
     * 适合启动视频播放、埋点统计
     */
    virtual void onAppear() {}

    /**
     * 当页面不可见时调用 (Did Disappear)
     * 包括：被新页面 Push 覆盖、或当前页面被 Pop 移除
     * 适合暂停视频、停止高频定时器
     */
    virtual void onDisappear() {}

    // ==========================================
    // 导航控制 (Navigation Control)
    // ==========================================

    /**
     * 设置关联的导航器
     * 注意：内部持有 weak_ptr 防止循环引用
     */
    void setNavigator(std::weak_ptr<LFNavigator> nav);

    /**
     * 获取导航器实例
     * 用于调用 navigator->pop() 或 push()
     */
    std::shared_ptr<LFNavigator> getNavigator() const;

protected:
    /**
     * 初始化页面默认样式
     * 1. 宽高撑满
     * 2. 白色背景
     * 3. 拦截点击
     */
    virtual void initPageStyle();

private:
    // 使用 weak_ptr，因为 Navigator 强持有 Page，Page 不能强持有 Navigator
    std::weak_ptr<LFNavigator> m_navigator;
};

#endif // LEAF_LFPAGE_H
