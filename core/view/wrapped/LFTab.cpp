//
// Created by Chen Tong on 2026/1/31.
//

#include "LFTab.h"
#include "view/base/LFText.h"
#include "view/base/LFImage.h"
#include "view/layout/LFLinear.h"

// 辅助函数：更新按钮样式
static void setTabButtonStyle(LFButton::Ptr btn, const std::string& iconPath, uint32_t textColor) {
    if (!btn) return;
    auto children = btn->getChildren();
    if (children.empty()) return;

    // 结构约定: Button -> Linear -> [Image, Text]
    auto container = std::dynamic_pointer_cast<LFLinear>(children[0]);
    if (!container) return;

    auto innerChildren = container->getChildren();

    // 更新图标
    if (!innerChildren.empty()) {
        if (auto img = std::dynamic_pointer_cast<LFImage>(innerChildren[0])) {
            img->setSrc(iconPath);
        }
    }
    // 更新文字
    if (innerChildren.size() > 1) {
        if (auto text = std::dynamic_pointer_cast<LFText>(innerChildren[1])) {
            text->setTextColor(textColor);
        }
    }
}

LFTab::Ptr LFTab::create() {
    auto tab = std::make_shared<LFTab>();
    tab->initLayout();
    return tab;
}

LFTab::LFTab() {
    // 默认垂直布局，因为上面是内容，下面是 Bar
    setOrientation(LFOrientation::Vertical);
    // 撑满父容器 (通常是 MainPage)
    matchParentWidth();
    matchParentHeight();
}

void LFTab::initLayout() {
    // 1. Content Area (核心区域)
    m_contentArea = LFBox::create();
    // 关键布局逻辑：占据剩余所有空间
    m_contentArea->setFlexGrow(1.0f);
    // 宽度撑满
    m_contentArea->matchParentWidth();
    // 开启裁剪 (防止页面内容溢出到底部栏)
    m_contentArea->setMasksToBounds(true);
    // 设置一个默认背景，避免透明穿透
    m_contentArea->setBackgroundColor(0xFFF5F5F5);

    addChild(m_contentArea);

    // 2. Bottom Bar (底部栏)
    m_bottomBar = LFLinear::createHorizontal();
    m_bottomBar->matchParentWidth();
    m_bottomBar->setHeight(BAR_HEIGHT); // 固定高度
    m_bottomBar->setBackgroundColor(0xFFFFFFFF); // 白底
    // 顶部分割线
    m_bottomBar->setBorder(0.5f, 0xFFE0E0E0);
    YGNodeStyleSetBorder(m_bottomBar->getYGNode(), YGEdgeBottom, 0);
    YGNodeStyleSetBorder(m_bottomBar->getYGNode(), YGEdgeLeft, 0);
    YGNodeStyleSetBorder(m_bottomBar->getYGNode(), YGEdgeRight, 0);

    // 按钮均分
    m_bottomBar->setDistribution(LFDistribution::SpaceAround);
    m_bottomBar->setAlignItems(YGAlignCenter);

    addChild(m_bottomBar);
}

void LFTab::addTab(const std::string& title,
                   LFPage::Ptr page,
                   const std::string& iconNormal,
                   const std::string& iconSelected) {
    if (!page) return;

    // 1. 数据记录
    TabItem item;
    item.page = page;
    item.title = title;
    item.iconNormal = iconNormal;
    item.iconSelected = iconSelected;
    m_tabItems.push_back(item);

    // 2. 页面布局处理
    // 关键：页面不需要知道自己在哪里，只需要撑满 ContentArea
    page->matchParentWidth();
    page->matchParentHeight();
    // 使用绝对定位重叠在一起，通过 setVisible 切换
    page->setPositionType(YGPositionTypeAbsolute);
    page->setPosition(YGEdgeAll, 0);
    page->setVisible(false); // 默认隐藏

    // 添加到 ContentArea
    m_contentArea->addChild(page);

    // 3. 创建底部按钮
    auto btn = LFButton::create();
    btn->setFlexGrow(1.0f);
    btn->matchParentHeight();
    btn->setClickEffect(LFClickEffect::None);
    btn->setBackgroundColor(LFButtonState::Normal, 0x00000000);
    btn->setBackgroundColor(LFButtonState::Pressed, 0x00000000);

    // 按钮内部结构
    auto inner = LFLinear::createVertical();
    inner->setGravity(LFAlignment::Center, LFAlignment::Center);
    inner->setSpacing(3.0f);
    inner->setTouchEnabled(false); // 穿透点击

    if (!iconNormal.empty()) {
        auto img = std::make_shared<LFImage>();
        img->setSrc(iconNormal);
        img->setWidth(24);
        img->setHeight(24);
        inner->addChild(img);
    }

    auto text = std::make_shared<LFText>();
    text->setText(title);
    text->setFontSize(10);
    text->setTextColor(COLOR_TEXT_NORMAL);
    text->setTextHAlign(LFTextHAlign::Center);
    inner->addChild(text);

    btn->addChild(inner, LFBoxAlign::Center);

    // 点击事件
    int targetIndex = m_tabItems.size() - 1;
    std::weak_ptr<LFTab> weakSelf = std::static_pointer_cast<LFTab>(shared_from_this());
    btn->setOnClick([weakSelf, targetIndex](LFButton* sender) {
        if (auto self = weakSelf.lock()) {
            self->selectTab(targetIndex);
        }
    });

    m_bottomBar->addChild(btn);
    m_tabButtons.push_back(btn);

    // 默认选中第一个
    if (m_currentIndex == -1) {
        selectTab(0);
    }
}

void LFTab::selectTab(int index) {
    if (index < 0 || index >= m_tabItems.size()) return;
    if (index == m_currentIndex) return;

    // 1. 隐藏旧页面
    if (m_currentIndex >= 0) {
        auto oldPage = m_tabItems[m_currentIndex].page;
        oldPage->setVisible(false);
        // 如果需要模拟生命周期，可以在这里手动调用 oldPage->onDisappear();
    }

    // 2. 显示新页面
    auto newPage = m_tabItems[index].page;
    newPage->setVisible(true);
    // 如果需要模拟生命周期，可以在这里手动调用 newPage->onAppear();

    m_currentIndex = index;

    // 3. 更新按钮样式
    updateTabButtonsState();
}

void LFTab::updateTabButtonsState() {
    for (int i = 0; i < m_tabButtons.size(); i++) {
        auto btn = m_tabButtons[i];
        const auto& item = m_tabItems[i];
        bool isSelected = (i == m_currentIndex);

        uint32_t color = isSelected ? COLOR_TEXT_SELECT : COLOR_TEXT_NORMAL;
        std::string icon = isSelected ?
                          (item.iconSelected.empty() ? item.iconNormal : item.iconSelected) :
                          item.iconNormal;

        setTabButtonStyle(btn, icon, color);
    }
}
