//
// Created by Chen Tong on 2026/2/2.
//

#include "ProfilePage.h"
#include "LFBox.h"
#include "LFLinear.h"
#include "LFImage.h"
#include "LFButton.h"
#include "AppUtils.h"

std::shared_ptr<ProfilePage> ProfilePage::create() {
    auto page = std::make_shared<ProfilePage>();
    page->initUI();
    return page;
}

void ProfilePage::initUI() {
    // 创建根容器
    auto root = LFBox::create();
    root->matchParentWidth();
    root->matchParentHeight();
    root->setBackgroundColor(0xFFF8F8F8); // 稍微带点灰的背景

    auto contentColumn = LFLinear::createVertical();
    contentColumn->matchParentWidth();
    contentColumn->wrapContentHeight();
    contentColumn->setGravity(LFAlignment::Center, LFAlignment::Center); // 水平居中
    contentColumn->setPadding(YGEdgeTop, 60); // 避开顶部状态栏

    // 头像
    auto avatar = std::make_shared<LFImage>();
    avatar->setSrc("avatar.jpg");
    float avatarSize = 100;
    avatar->setWidth(avatarSize);
    avatar->setHeight(avatarSize);
    avatar->setBorderRadius(avatarSize / 2.0f);
    avatar->setBorder(3.0f, 0xFFFFFFFF);
    avatar->setFit(LFImageFit::Fill);
    avatar->setShadow(0, 5, 10, 0, 0x33000000);
    contentColumn->addChild(avatar);

    // 名字
    auto nameText = AppUtils::createLabel("Developer", 20, 0xFF222222, true);
    nameText->setMargin(YGEdgeTop, 30);
    contentColumn->addChild(nameText);

    // 职位
    auto jobText = AppUtils::createLabel("Full-stack Engineer & UI Designer", 12, 0xFF888888);
    jobText->setMargin(YGEdgeTop, 20);
    contentColumn->addChild(jobText);

    // 统计数据行
    auto statsRow = LFLinear::createHorizontal();
    statsRow->setWidthPercent(80.0f);
    statsRow->setMargin(YGEdgeTop, 24);
    statsRow->setDistribution(LFDistribution::SpaceEvenly);

    auto makeStatItem = [](const std::string& count, const std::string& label) {
        auto container = LFLinear::createVertical();
        container->setWidth(80);
        container->setHeight(40);
        container->setGravity(LFAlignment::Center, LFAlignment::Center);

        auto numTxt = AppUtils::createLabel(count, 14, 0xFF000000, true);
        numTxt->wrapContentWidth();
        auto labelTxt = AppUtils::createLabel(label, 10, 0xFF999999);
        labelTxt->wrapContentWidth();
        labelTxt->setMargin(YGEdgeTop, 10);

        container->addChild(numTxt);
        container->addChild(labelTxt);
        return container;
    };

    statsRow->addChild(makeStatItem("99+", "Posts"));
    statsRow->addChild(makeStatItem("12k", "Followers"));
    statsRow->addChild(makeStatItem("350", "Following"));
    contentColumn->addChild(statsRow);

    // 编辑按钮
    auto btn = LFButton::create();
    btn->setWidth(130);
    btn->setHeight(40);
    btn->setMargin(YGEdgeTop, 30);
    btn->setBackgroundColor(LFButtonState::Normal, 0xFF007AFF);
    btn->setBackgroundColor(LFButtonState::Pressed, 0xFF0056B3);
    btn->setBorderRadius(6);
    btn->setText("Edit Profile");
    btn->setFontSize(12);
    btn->setTextColor(0xFFFFFFFF);
    btn->setShadow(0, 3, 6, 0, 0x40000000);
    btn->setOnTap([](const LFPoint& location) {
        LF_LOGI("tap edit profile");
    });
    contentColumn->addChild(btn);

    root->addChild(contentColumn, LFBoxAlign::TopLeft);

    // 底部信息
    auto bottomColumn = LFLinear::createVertical();
    bottomColumn->matchParentWidth();
    bottomColumn->wrapContentHeight();
    bottomColumn->setGravity(LFAlignment::Start, LFAlignment::Center);
    bottomColumn->setSpacing(20);

    auto infoText = AppUtils::createLabel("Next-Gen Cross-Platform UI Engine\nPowered by C++", 12, 0xFF007AFF);
    infoText->matchParentWidth();
    infoText->setLineHeight(1.5f);

    auto contactText = AppUtils::createLabel("contact@example.com", 10, 0xFFAAAAAA);
    contactText->wrapContentWidth();
    contactText->setTextHAlign(LFTextHAlign::Left);

    bottomColumn->addChild(infoText);
    bottomColumn->addChild(contactText);

    root->addChild(bottomColumn, LFBoxAlign::BottomCenter, 0, -40);

    this->addChild(root);
}
