//
// Created by Chen Tong on 2026/1/31.
//

#ifndef LEAF_LFTAB_H
#define LEAF_LFTAB_H

#include "view/layout/LFLinear.h"
#include "view/layout/LFBox.h"
#include "view/base/LFPage.h"
#include "LFButton.h"

/**
 * 底部标签栏容器 (Tab Container)
 *
 * 继承自 LFLinear (垂直布局)，结构如下：
 * [ Content Area (LFBox, FlexGrow=1) ] -> 存放 LFPage
 * [ Bottom Bar   (LFLinear, Height=Fixed) ] -> 存放按钮
 */
class LFTab : public LFLinear {
public:
    using Ptr = std::shared_ptr<LFTab>;

    static Ptr create();

    LFTab();
    virtual ~LFTab() = default;

    /**
     * 添加 Tab 页
     * @param title 标题
     * @param page 页面实例 (必须是 LFPage::Ptr)
     * @param iconNormal 未选中图标路径
     * @param iconSelected 选中图标路径
     */
    void addTab(const std::string& title,
                LFPage::Ptr page,
                const std::string& iconNormal = "",
                const std::string& iconSelected = "");

    /**
     * 切换 Tab
     */
    void selectTab(int index);

    int getCurrentIndex() const { return m_currentIndex; }

    /**
     * 获取内容区域容器，方便做特殊处理（如背景色）
     */
    LFBox::Ptr getContentArea() const { return m_contentArea; }

private:
    void initLayout();
    void updateTabButtonsState();

    // UI 结构
    LFBox::Ptr m_contentArea;  // 核心内容区
    std::shared_ptr<LFLinear> m_bottomBar; // 底部导航栏

    // 数据结构
    struct TabItem {
        LFPage::Ptr page;      // 你的建议：直接持有 LFPage
        std::string title;
        std::string iconNormal;
        std::string iconSelected;
    };

    // 页面列表 (m_tabContents)
    std::vector<TabItem> m_tabItems;

    // 底部按钮引用
    std::vector<LFButton::Ptr> m_tabButtons;

    int m_currentIndex = -1;

    // 常量
    const float BAR_HEIGHT = 56.0f;
    const uint32_t COLOR_TEXT_NORMAL = 0xFF999999;
    const uint32_t COLOR_TEXT_SELECT = 0xFF007AFF;
};

#endif // LEAF_LFTAB_H
