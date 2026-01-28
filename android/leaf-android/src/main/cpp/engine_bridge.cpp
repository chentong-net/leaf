#define NANOVG_GLES3_IMPLEMENTATION

#include "LFEngine.h"

// 辅助函数：快速创建文本节点 (Outside extern "C" block)
std::shared_ptr<LFText> createText(const std::string& content, float fontSize, uint32_t color, bool isBold = false) {
    auto text = std::make_shared<LFText>();
    text->setText(content);
    text->setFontSize(fontSize);
    text->setTextColor(color);
    return text;
}

extern "C" {

void leaf_init(std::function<std::string(const char *path)> loader) {
    // 1. 创建 NanoVG 上下文
    int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES;
    NVGcontext* vg = nvgCreateGLES3(flags);

    if (!vg) return;

    // 2. 初始化引擎核心
    LFEngine::getInstance().init(vg);

    // 4. 加载字体 (直接使用 Android 系统字体)
    // 优先加载 Roboto，如果失败则回退到 NotoSansCJK (支持中文)
    std::string fontData = loader("fonts/MapleMonoNormalNL-Regular.ttf");
    if (nvgCreateFontMem(vg, "sans", (unsigned char*) fontData.data(), fontData.size(), 0) == -1) {
        // If failed (e.g. some custom ROMs)
        nvgCreateFont(vg, "sans", "/system/fonts/NotoSansCJK-Regular.ttc");
    }

    // 3. 配置资源加载器
    LFResourceProvider::getInstance().setImageLoader(
            [loader](const std::string& path, std::function<void(std::shared_ptr<LFImageData>)> callback) {
                std::string raw = loader(path.c_str());
                if (raw.empty()) {
                    callback(nullptr);
                    return;
                }

                // 这里仅做数据透传演示
                auto img = std::make_shared<LFImageData>();
                img->size = raw.size();
                img->data = (unsigned char*)malloc(img->size);
                memcpy(img->data, raw.data(), img->size);

                // 硬编码一下 avatar.jpg 的尺寸，防止 layout 也就是 measure 算错
                // 实际解码后会有真实宽高
                img->width = 200;
                img->height = 200;

                callback(img);
            }
    );
}

void leaf_update_size(float w, float h, float d) {
    LFEngine::getInstance().setWindowSize((float)w, (float)h, d);
}

void leaf_render() {
    // Update logic (gesture timing, etc.)
    LFEngine::getInstance().update(0.016f);  // Assume 60fps (~16ms)

    // Render frame
    LFEngine::getInstance().render();
}

std::shared_ptr<LFNode> buildTouchNode() {
    // ======================================
    // 手势系统完整测试 Demo (适配版)
    // ======================================

    // 定义移动端标准参数 (Logical Pixels)
    float cardHeight = 100.0f;    // 这种高度更适合列表项
    float cardRadius = 12.0f;     // 圆润但不过分
    float stdPadding = 16.0f;     // 标准边距

    auto root = LFScrollView::createVertical();
    root->matchParentWidth();
    root->matchParentHeight();
    root->setBackgroundColor(0xFF121212);  // Material Design 深色背景
    root->setScrollBarEnabled(false);
    root->setBounces(false);

    // 主内容区域 - 垂直线性布局
    auto contentColumn = LFLinear::createVertical();
    contentColumn->matchParentWidth();
    contentColumn->wrapContentHeight();
    contentColumn->setGravity(LFAlignment::Start, LFAlignment::Center); // 水平居中，垂直从头开始
    contentColumn->setSpacing(12.0f);   // 列表项间距
    contentColumn->setPadding(YGEdgeAll, stdPadding);
    // 留出顶部安全区域 (Status Bar)
    contentColumn->setPadding(YGEdgeTop, 44.0f);
    // 留出底部安全区域 (Home Indicator)
    contentColumn->setPadding(YGEdgeBottom, 34.0f);

    // 辅助函数：快速创建通用卡片样式
    auto createCard = [&](uint32_t bgColor) {
        auto box = LFBox::create();
        box->setWidthPercent(94.0f); // 宽度占屏幕 94%，自动适配不同机型
        box->setHeight(cardHeight);
        box->setBackgroundColor(bgColor);
        box->setBorderRadius(cardRadius);
        // 细腻的阴影: y=4, blur=10
        box->setShadow(0, 4, 10, 0, 0x40000000);
        return box;
    };

    // 辅助函数：创建居中文本
    auto createLabel = [&](const std::string& text, float size = 16) {
        auto txt = createText(text, size, 0xFFFFFFFF, true); // 加粗
        txt->setTextHAlign(LFTextHAlign::Center);
        txt->setTextVAlign(LFTextVAlign::Center);
        return txt;
    };

    // ====================
    // 1. Tap 手势测试
    // ====================
    auto tapBox = createCard(0xFF4CAF50); // Green

    auto tapText = createLabel("Tap / Double Tap");
    tapBox->addChild(tapText, LFBoxAlign::Center);

    tapBox->setOnTap([tapBox](const LFPoint& location) {
        // 反馈：变红
        tapBox->setBackgroundColor(0xFFE53935);
        // 延时还原颜色 (模拟点击态) - 实际项目中建议用动画系统
    });

    tapBox->setOnDoubleTap([tapBox](const LFPoint& location) {
        tapBox->setBackgroundColor(0xFF2196F3); // Blue
    });

    contentColumn->addChild(tapBox);

    // ====================
    // 2. Long Press 手势测试
    // ====================
    auto longPressBox = createCard(0xFFFF9800); // Orange

    auto longPressText = createLabel("Long Press");
    longPressBox->addChild(longPressText, LFBoxAlign::Center);

    longPressBox->setOnLongPress([longPressBox](const LFPoint& location) {
        // 长按反馈：变深色
        longPressBox->setBackgroundColor(0xFFE65100);
    });

    contentColumn->addChild(longPressBox);

    // ====================
    // 3. Pan (拖拽) 手势测试
    // ====================
    // 拖拽区域不用 Card 样式，用一个专门的圆形，更有趣
    auto panContainer = LFBox::create();
    panContainer->setWidthPercent(100.0f);
    panContainer->setHeight(150.0f); // 给一个较大的活动区域
    // panContainer->setBackgroundColor(0xFF1E1E1E); // 可选：给个淡背景

    auto panBall = LFBox::create();
    panBall->setWidth(80.0f);
    panBall->setHeight(80.0f);
    panBall->setBackgroundColor(0xFF2196F3);
    panBall->setBorderRadius(40.0f);
    panBall->setShadow(0, 6, 12, 0, 0x50000000);

    auto panText = createLabel("Drag", 14.0f);
    panBall->addChild(panText, LFBoxAlign::Center);

    // 居中放置
    panContainer->addChild(panBall, LFBoxAlign::Center);

    panBall->setOnPan(
            [panBall](const LFPoint& delta, const LFPoint& velocity) {
                auto t = panBall->getTransform();
                panBall->setTranslate(t.translateX + delta.x, t.translateY + delta.y);
            },
            nullptr, nullptr // 忽略 start/end
    );

    contentColumn->addChild(panContainer);

    // ====================
    // 4. Pinch (缩放) & Rotate (旋转)
    //    合并演示，更节省空间
    // ====================
    auto gestureBox = createCard(0xFF9C27B0); // Purple
    gestureBox->setHeight(180.0f); // 稍微大一点方便操作

    auto gestureText = createLabel("Pinch & Rotate");
    gestureBox->addChild(gestureText, LFBoxAlign::Center);

    gestureBox->setOnPinch(
            [gestureBox](float scale, const LFPoint& focal) {
                auto t = gestureBox->getTransform();
                // 阻尼感：让缩放不那么剧烈
                float s = t.scaleX * scale;
                s = std::max(0.8f, std::min(1.5f, s)); // 限制范围
                gestureBox->setScale(s, s);
            }, nullptr, nullptr
    );

    gestureBox->setOnRotate(
            [gestureBox](float angle, const LFPoint& focal) {
                auto t = gestureBox->getTransform();
                float deg = angle * 180.0f / M_PI;
                gestureBox->setRotate(t.rotate + deg);
            }, nullptr, nullptr
    );

    contentColumn->addChild(gestureBox);

    // ====================
    // 6. Swipe (轻扫) 手势测试
    // ====================
    auto swipeRow = LFLinear::createHorizontal();
    swipeRow->setWidthPercent(94.0f);
    swipeRow->setHeight(80.0f);
    swipeRow->setDistribution(LFDistribution::SpaceBetween);

    auto createSwipeItem = [&](const std::string& txt, int dir, uint32_t color) {
        auto box = LFBox::create();
        box->setWidthPercent(48.0f); // 两个并排，各占48%
        box->matchParentHeight();
        box->setBackgroundColor(color);
        box->setBorderRadius(cardRadius);
        box->addChild(createLabel(txt, 14.0f), LFBoxAlign::Center);

        box->setOnSwipe([box](int d, const LFPoint& v){
            box->setOpacity(0.5f); // 视觉反馈
            // TODO: 最好有个 Timer 恢复透明度
        }, dir);
        return box;
    };

    swipeRow->addChild(createSwipeItem("Swipe H", 1|2, 0xFF00BCD4));
    swipeRow->addChild(createSwipeItem("Swipe V", 4|8, 0xFF009688));

    contentColumn->addChild(swipeRow);

    // ====================
    // 7. 竞技场 (Arena)
    // ====================
    auto arenaBox = createCard(0xFFFF5722); // Deep Orange
    arenaBox->setHeight(120.0f);

    auto arenaText = createLabel("Arena: Tap vs Pan\n(Try scrolling vs clicking)");
    arenaText->setLineHeight(1.4f);
    arenaBox->addChild(arenaText, LFBoxAlign::Center);

    arenaBox->setOnTap([arenaBox](const LFPoint& p){
        arenaBox->setBackgroundColor(0xFF2196F3);
    });

    arenaBox->setOnPan(
            [arenaBox](const LFPoint& d, const LFPoint& v) {
                auto t = arenaBox->getTransform();
                arenaBox->setTranslate(t.translateX + d.x, t.translateY + d.y);
            }, nullptr, nullptr
    );

    contentColumn->addChild(arenaBox);

    // 最后添加到底部
    root->addChild(contentColumn);

    return root;
}

std::shared_ptr<LFNode> buildRootNode(std::shared_ptr<LFNavigator> navigator, std::shared_ptr<LFNode> newPage) {
    // 创建根容器
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

    auto nameText = createText("Developer", 20, 0xFF222222, true);
    nameText->setTextHAlign(LFTextHAlign::Center);
    nameText->setTextVAlign(LFTextVAlign::Center);
    nameText->setMargin(YGEdgeTop, 30); // 与头像的间距
    contentColumn->addChild(nameText);

    auto jobText = createText("Full-stack Engineer & UI Designer", 12, 0xFF888888);
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

        auto numTxt = createText(count, 14, 0xFF000000, true);
        numTxt->wrapContentWidth();
        numTxt->setTextHAlign(LFTextHAlign::Center);
        numTxt->setTextVAlign(LFTextVAlign::Center);
        auto labelTxt = createText(label, 10, 0xFF999999);
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
    btn->setOnTap([btn, navigator, newPage](const LFPoint& location) {
        LF_LOGI("tap");
        auto nextPage = LFPage::create();
        nextPage->setBackgroundColor(0xFFF5F5F5);
        nextPage->addChild(newPage);
        navigator->push(nextPage);
    });

    contentColumn->addChild(btn);

    root->addChild(contentColumn, LFBoxAlign::TopLeft);

    auto bottomColumn = LFLinear::createVertical();
    bottomColumn->matchParentWidth();
    bottomColumn->wrapContentHeight();
    bottomColumn->setGravity(LFAlignment::Start, LFAlignment::Center);
    bottomColumn->setSpacing(20);

    auto infoText = createText("Next-Gen Cross-Platform UI Engine\nPowered by C++", 12, 0xFF007AFF);
    infoText->matchParentWidth();
    infoText->setLineHeight(1.5f);
    infoText->setTextVAlign(LFTextVAlign::Center);
    infoText->setTextHAlign(LFTextHAlign::Center);

    auto contactText = createText("contact@example.com", 10, 0xFFAAAAAA);
    contactText->wrapContentWidth();
    contactText->setTextVAlign(LFTextVAlign::Center);
    contactText->setTextHAlign(LFTextHAlign::Left);

    bottomColumn->addChild(infoText);
    bottomColumn->addChild(contactText);


    root->addChild(bottomColumn, LFBoxAlign::BottomCenter, 0, -40);
    return root;
}

void leaf_eval_js(const char *code) {

    auto navigator = LFNavigator::create();

    auto rootPage = LFPage::create();
    rootPage->setBackgroundColor(0xFFF5F5F5);
    auto root = buildRootNode(navigator, buildTouchNode());
    rootPage->addChild(root);
    navigator->push(rootPage);

    // 提交
    LFEngine::getInstance().setRoot(navigator);
}

}