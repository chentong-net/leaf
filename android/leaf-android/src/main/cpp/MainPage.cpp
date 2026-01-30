//
// Created by Chen Tong on 2026/1/31.
//

#include "LFEngine.h"

#include "LFEngine.h"
#include "LFNavigator.h"
#include "LFPage.h"
#include "LFTab.h"
#include "LFGrid.h"
#include "LFScrollView.h"
#include "LFLinear.h"
#include "LFBox.h"
#include "LFText.h"
#include "LFImage.h"
#include "LFButton.h"

// ==========================================
// 辅助工具函数：减少重复代码
// ==========================================
static std::shared_ptr<LFText> createLabel(const std::string& text, float size, uint32_t color, bool bold = false) {
    auto t = std::make_shared<LFText>();
    t->setText(text);
    t->setFontSize(size);
    t->setTextColor(color);
    t->setTextHAlign(LFTextHAlign::Center);
    t->setTextVAlign(LFTextVAlign::Center);
    // 如果支持粗体可以在这里扩展 setFontFamily
    return t;
}

// ==========================================
// 1. 构建开发者信息组件 (Tab 2)
// ==========================================
LFPage::Ptr buildDevInfoPage() {
    // 创建根容器
    auto page = LFPage::create();
    auto root = LFBox::create();
    root->matchParentWidth();
    root->matchParentHeight();
    root->setBackgroundColor(0xFFF8F8F8); //稍微带点灰的背景，更有质感

    auto contentColumn = LFLinear::createVertical();
    contentColumn->matchParentWidth();
    contentColumn->wrapContentHeight();
    contentColumn->setGravity(LFAlignment::Center, LFAlignment::Center); // 水平居中
    contentColumn->setPadding(YGEdgeTop, 60); // 避开顶部状态栏，留出大片空白

    auto avatar = std::make_shared<LFImage>();
    avatar->setSrc("avatar.jpg");
    float avatarSize = 100;
    avatar->setWidth(avatarSize);
    avatar->setHeight(avatarSize);
    avatar->setBorderRadius(avatarSize / 2.0f);
    avatar->setBorder(3.0f, 0xFFFFFFFF);
    avatar->setFit(LFImageFit::Cover);
    avatar->setShadow(0, 5, 10, 0, 0x33000000);

    contentColumn->addChild(avatar);

    auto nameText = createLabel("Developer", 20, 0xFF222222, true);
    nameText->setTextHAlign(LFTextHAlign::Center);
    nameText->setTextVAlign(LFTextVAlign::Center);
    nameText->setMargin(YGEdgeTop, 30); // 与头像的间距
    contentColumn->addChild(nameText);

    auto jobText = createLabel("Full-stack Engineer & UI Designer", 12, 0xFF888888);
    jobText->setMargin(YGEdgeTop, 20);
    jobText->setTextHAlign(LFTextHAlign::Center);
    jobText->setTextVAlign(LFTextVAlign::Center);
    contentColumn->addChild(jobText);

    auto statsRow = LFLinear::createHorizontal();
    statsRow->setWidthPercent(80.0f); // 宽度占屏幕 80%
    statsRow->setMargin(YGEdgeTop, 24);
    statsRow->setDistribution(LFDistribution::SpaceEvenly); // 等间距分布

    // 创建单个统计项
    auto makeStatItem = [](const std::string& count, const std::string& label) {
        auto container = LFLinear::createVertical();
        container->setWidth(80);
        container->setHeight(40);
        container->setGravity(LFAlignment::Center, LFAlignment::Center);

        auto numTxt = createLabel(count, 14, 0xFF000000, true);
        numTxt->wrapContentWidth();
        numTxt->setTextHAlign(LFTextHAlign::Center);
        numTxt->setTextVAlign(LFTextVAlign::Center);
        auto labelTxt = createLabel(label, 10, 0xFF999999);
        labelTxt->wrapContentWidth();
        labelTxt->setTextHAlign(LFTextHAlign::Center);
        labelTxt->setTextVAlign(LFTextVAlign::Center);
        labelTxt->setMargin(YGEdgeTop, 10);

        container->addChild(numTxt);
        container->addChild(labelTxt);
        return container;
    };

    statsRow->addChild(makeStatItem("99+", "Posts"));
    statsRow->addChild(makeStatItem("12k", "Followers"));
    statsRow->addChild(makeStatItem("350", "Following"));

    contentColumn->addChild(statsRow);

    auto btn = LFButton::create();
    btn->setWidth(130); // 按钮宽度
    btn->setHeight(40); // 按钮高度
    btn->setMargin(YGEdgeTop, 30);
    btn->setBackgroundColor(LFButtonState::Normal, 0xFF007AFF); // 纯黑背景
    btn->setBackgroundColor(LFButtonState::Pressed, 0xFF0056B3);
    btn->setBackgroundColor(LFButtonState::Disabled, 0xFFB0B0B5);
    btn->setBorderRadius(6); // 小圆角
    btn->setText("Edit Profile");
    btn->setFontSize(12);
    btn->setTextColor(0xFFFFFFFF);
    btn->setShadow(0, 3, 6, 0, 0x40000000);
    btn->setOnTap([btn](const LFPoint& location) {
        LF_LOGI("tap");
    });

    contentColumn->addChild(btn);

    root->addChild(contentColumn, LFBoxAlign::TopLeft);

    auto bottomColumn = LFLinear::createVertical();
    bottomColumn->matchParentWidth();
    bottomColumn->wrapContentHeight();
    bottomColumn->setGravity(LFAlignment::Start, LFAlignment::Center);
    bottomColumn->setSpacing(20);

    auto infoText = createLabel("Next-Gen Cross-Platform UI Engine\nPowered by C++", 12, 0xFF007AFF);
    infoText->matchParentWidth();
    infoText->setLineHeight(1.5f);
    infoText->setTextVAlign(LFTextVAlign::Center);
    infoText->setTextHAlign(LFTextHAlign::Center);

    auto contactText = createLabel("contact@example.com", 10, 0xFFAAAAAA);
    contactText->wrapContentWidth();
    contactText->setTextVAlign(LFTextVAlign::Center);
    contactText->setTextHAlign(LFTextHAlign::Left);

    bottomColumn->addChild(infoText);
    bottomColumn->addChild(contactText);


    root->addChild(bottomColumn, LFBoxAlign::BottomCenter, 0, -40);
    page->addChild(root);
    return page;
}

// ==========================================
// 2. 构建书架组件 (Tab 1)
// ==========================================
LFPage::Ptr buildBookshelfPage() {
    auto page = LFPage::create();
    page->setBackgroundColor(0xFFF0F0F0); // 1. 页面背景：浅灰

    // 顶部导航栏
    auto navbar = LFLinear::createHorizontal();
    navbar->matchParentWidth();
    navbar->setHeight(56);
    navbar->setBackgroundColor(0xFFFFFFFF); // 导航栏：白色
    navbar->setPadding(YGEdgeHorizontal, 16);
    navbar->setGravity(LFAlignment::Start, LFAlignment::Center);

    // 加个边框确保能看见
    navbar->setBorder(1.0f, 0xFFE0E0E0);
    YGNodeStyleSetBorder(navbar->getYGNode(), YGEdgeBottom, 0);

    auto title = createLabel("我的书架", 18, 0xFF000000); // 黑色文字
    navbar->addChild(title);

    // 滚动区域
    auto scrollView = LFScrollView::createVertical();
    scrollView->setFlexGrow(1.0f);
    // scrollView->setFlexShrink(1.0f); // 如果空间不足(比如被键盘顶起)，允许缩小
    scrollView->setFlexBasis(0); // 初始计算基准为 0，完全依赖剩余空间
    scrollView->matchParentWidth();
    scrollView->setBounces(false);
    scrollView->setScrollBarEnabled(false);

    // 网格布局
    // 3列，间距 15dp
    auto grid = LFGrid::create(3, 15.0f);
    grid->matchParentWidth();
    grid->setPadding(YGEdgeAll, 15.0f);

    // 填充书籍数据
    for (int i = 0; i < 15; i++) {
        // 内部布局 (垂直)
        auto itemLayout = LFLinear::createVertical();
        itemLayout->matchParentWidth();
        itemLayout->wrapContentHeight();
        itemLayout->setGravity(LFAlignment::Center, LFAlignment::Center);
        itemLayout->setSpacing(15.0f);

        // 1. 封面
        auto cover = LFBox::create();
        cover->matchParentWidth();
        cover->setAspectRatio(0.75f);
        // 随机颜色封面，确保能看见
        uint32_t colors[] = {0xFFFFABAB, 0xFFFFFFCC, 0xFFCCFFCC, 0xFFCCCCFF};
        cover->setBackgroundColor(colors[i % 4]);
        cover->setRadius(6.0f);
        cover->setShadow(0, 2, 5, 0, 0x33000000);

        // 封面上的文字
        auto coverText = createLabel("Book " + std::to_string(i+1), 14, 0xFF555555);
        cover->addChild(coverText, LFBoxAlign::Center);

        // 2. 书名
        auto nameText = createLabel("测试书籍 " + std::to_string(i), 12, 0xFF333333);

        // 组装
        itemLayout->addChild(cover);
        itemLayout->addChild(nameText);

        // 点击测试
        itemLayout->setOnTap([i](const LFPoint& point){
            LF_LOGI("Clicked Book: %d", i);
        });

        grid->addChild(itemLayout);
    }

    scrollView->addChild(grid);

    page->addChild(navbar);
    page->addChild(scrollView);

    return page;
}

// ==========================================
// 3. 构建 Tab 容器 (应用入口)
// ==========================================
LFNode::Ptr buildTabContainer() {
    // 1. 创建 Tab 组件
    auto tab = LFTab::create();

    // 2. 添加子页面
    // 目前没有 Icon，传入空字符串，LFTab 会自动只显示文字
    tab->addTab("书架", buildBookshelfPage(), "", "");
    tab->addTab("开发者信息", buildDevInfoPage(), "", "");

    // 3. 创建导航器 (Navigator)
    // 这是架构的关键：我们在最外层包裹一个 Navigator。
    // 这样当你在“书架”点击书本时，可以调用 navigator->push(readerPage)
    // 这个新的 readerPage 会覆盖掉整个 TabBar，这是符合移动端规范的。
    auto navigator = LFNavigator::create();

    // 4. 创建一个宿主 Page 来容纳 Tab
    auto rootPage = LFPage::create();
    rootPage->addChild(tab); // Tab 撑满页面

    // 5. 将宿主页推入栈底
    navigator->push(rootPage, false); // false = 无动画，作为第一页

    return navigator;
}
