//
// Created by Chen Tong on 2026/1/29.
//

#include "LFPage.h"
// #include "LFNavigator.h" // 如果需要调用 Navigator 的方法，可能需要这个，但此处仅用于类型完整性

LFPage::Ptr LFPage::create() {
    auto page = std::make_shared<LFPage>();
    page->initPageStyle();
    return page;
}

LFPage::LFPage() {
    // 构造函数保持轻量
}

void LFPage::setNavigator(std::weak_ptr<LFNavigator> nav) {
    m_navigator = nav;
}

std::shared_ptr<LFNavigator> LFPage::getNavigator() const {
    return m_navigator.lock();
}

void LFPage::initPageStyle() {
    // 1. 布局：默认撑满父容器 (Navigator)
    // 这里的逻辑依赖于 Navigator (LFBox) 会将其作为子节点布局
    matchParentWidth();
    matchParentHeight();

    // 2. 视觉：默认不透明背景
    // 重要：如果 Page 是透明的，用户会通过当前页看到上一页的内容（视觉重叠）。
    // 除非你是做弹窗 (Dialog) 模式，否则普通页面必须有背景色。
    setBackgroundColor(0xFFFFFFFF); // White

    // 3. 交互：拦截点击事件 (Event Interception)
    // 这是一个常见的 UI 引擎坑点：
    // 如果 Page 没有设置点击监听，触摸事件会“穿透”过去，
    // 导致用户点击当前页空白处时，意外触发了底下一页的按钮。
    // 解决方案：注册一个空的 OnTap 监听，吞掉冒泡事件。
    setOnTap([](const LFPoint& point) {
        // Do nothing, just intercept.
        // LF_LOGD("LFPage: Touch intercepted by background.");
    });
}
