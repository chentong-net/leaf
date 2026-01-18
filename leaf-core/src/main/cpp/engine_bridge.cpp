#define NANOVG_GLES3_IMPLEMENTATION

#include "LFEngine.h"

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
    if (nvgCreateFont(vg, "sans", "/system/fonts/Roboto-Regular.ttf") == -1) {
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

void leaf_update_size(int w, int h, float d) {
    LFEngine::getInstance().setWindowSize((float)w, (float)h);
}

void leaf_render() {
    LFEngine::getInstance().render();
}

// 辅助函数：快速创建文本节点
std::shared_ptr<LFText> createText(const std::string& content, float fontSize, uint32_t color, bool isBold = false) {
    auto text = std::make_shared<LFText>();
    text->setText(content);
    text->setFontSize(fontSize);
    text->setTextColor(color);
    return text;
}

void leaf_eval_js(const char *code) {
    // 创建根容器
    auto root = LFBox::create();
    root->matchParentWidth();
    root->matchParentHeight();
    root->setBackgroundColor(0xFFF8F8F8); //稍微带点灰的背景，更有质感

    auto contentColumn = LFLinear::createVertical();
    contentColumn->matchParentWidth();
    contentColumn->wrapContentHeight();
    contentColumn->setGravity(LFAlignment::Center, LFAlignment::Center); // 水平居中
    contentColumn->setPadding(YGEdgeTop, 180); // 避开顶部状态栏，留出大片空白

    auto avatar = std::make_shared<LFImage>();
    avatar->setSrc("avatar.jpg");
    float avatarSize = 280.0f;
    avatar->setWidth(avatarSize);
    avatar->setHeight(avatarSize);
    avatar->setBorderRadius(avatarSize / 2.0f);
    avatar->setBorder(8.0f, 0xFFFFFFFF);
    avatar->setFit(LFImageFit::Cover);
    avatar->setShadow(0, 15, 30, 0, 0x33000000);

    contentColumn->addChild(avatar);

    auto nameText = createText("Chen Tong", 72.0f, 0xFF222222, true);
    nameText->setTextHAlign(LFTextHAlign::Center);
    nameText->setTextVAlign(LFTextVAlign::Center);
    nameText->setMargin(YGEdgeTop, 50); // 与头像的间距
    contentColumn->addChild(nameText);

    auto jobText = createText("Full-stack Engineer & UI Designer", 42.0f, 0xFF888888);
    jobText->setMargin(YGEdgeTop, 20);
    contentColumn->addChild(jobText);

    auto statsRow = LFLinear::createHorizontal();
    statsRow->setWidthPercent(80.0f); // 宽度占屏幕 80%
    statsRow->setMargin(YGEdgeTop, 80);
    statsRow->setDistribution(LFDistribution::SpaceEvenly); // 等间距分布

    // 创建单个统计项
    auto makeStatItem = [](const std::string& count, const std::string& label) {
        auto container = LFLinear::createVertical();
        container->setGravity(LFAlignment::Center, LFAlignment::Center);

        auto numTxt = createText(count, 56.0f, 0xFF000000, true);
        numTxt->wrapContentWidth();
        numTxt->setTextHAlign(LFTextHAlign::Center);
        numTxt->setTextVAlign(LFTextVAlign::Center);
        auto labelTxt = createText(label, 36.0f, 0xFF999999);
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

    auto btn = LFLinear::createVertical();
    btn->setWidth(500.0f); // 按钮宽度
    btn->setHeight(130.0f); // 按钮高度
    btn->setMargin(YGEdgeTop, 90);
    btn->setBackgroundColor(0xFF000000); // 纯黑背景
    btn->setBorderRadius(20.0f); // 小圆角
    btn->setGravity(LFAlignment::Center, LFAlignment::Center);
    btn->setShadow(0, 10, 20, 0, 0x40000000);

    auto btnText = createText("Edit Profile", 48.0f, 0xFFFFFFFF);
    btn->addChild(btnText);

    contentColumn->addChild(btn);

    root->addChild(contentColumn, LFBoxAlign::TopLeft);

    auto bottomColumn = LFLinear::createVertical();
    bottomColumn->matchParentWidth();
    bottomColumn->wrapContentHeight();
    bottomColumn->setGravity(LFAlignment::Start, LFAlignment::Center);
    bottomColumn->setSpacing(50);

    auto infoText = createText("Next-Gen Cross-Platform UI Engine\nPowered by C++, QuickJS, Yoga, and NanoVG", 40.0f, 0xFF007AFF);
    infoText->wrapContentWidth();
    infoText->setLineHeight(1.5f);
    infoText->setTextVAlign(LFTextVAlign::Center);
    infoText->setTextHAlign(LFTextHAlign::Center);

    auto contactText = createText("contact@chentong.net", 36.0f, 0xFFAAAAAA);
    contactText->wrapContentWidth();
    contactText->setTextVAlign(LFTextVAlign::Center);
    contactText->setTextHAlign(LFTextHAlign::Left);

    bottomColumn->addChild(infoText);
    bottomColumn->addChild(contactText);


    root->addChild(bottomColumn, LFBoxAlign::BottomCenter, 0, -100);

    // 3. 提交给引擎
    LFEngine::getInstance().setRoot(root);
}

}