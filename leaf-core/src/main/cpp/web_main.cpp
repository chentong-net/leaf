//
// Created by Chen Tong on 2026/1/22.
//

#define NANOVG_GLES3_IMPLEMENTATION

#include "LFEngine.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/fetch.h>
#include "event/LFEvent.h"
#include "event/LFEventDispatcher.h"

static const char* CANVAS_ID = "#canvas";
double dpr = 1.0f;

std::shared_ptr<LFText> createText(const std::string& content, float fontSize, uint32_t color, bool isBold = false) {
    auto text = std::make_shared<LFText>();
    text->setText(content);
    text->setFontSize(fontSize);
    text->setTextColor(color);
    return text;
}

std::shared_ptr<LFNode> build() {
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

    auto btn = LFLinear::createVertical();
    btn->setWidth(130); // 按钮宽度
    btn->setHeight(40); // 按钮高度
    btn->setMargin(YGEdgeTop, 30);
    btn->setBackgroundColor(0xFF000000); // 纯黑背景
    btn->setBorderRadius(6); // 小圆角
    btn->setGravity(LFAlignment::Center, LFAlignment::Center);
    btn->setShadow(0, 3, 6, 0, 0x40000000);

    auto btnText = createText("Edit Profile", 12, 0xFFFFFFFF);
    btnText->setTextHAlign(LFTextHAlign::Center);
    btnText->setTextVAlign(LFTextVAlign::Center);
    btn->addChild(btnText);

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

void update_canvas_size() {
    double cssW, cssH;
    // 1. 获取 HTML 元素目前的 CSS 显示尺寸 (比如 800x600)
    emscripten_get_element_css_size(CANVAS_ID, &cssW, &cssH);

    // 2. 获取设备像素比 (Retina 屏通常是 2.0)
    dpr = emscripten_get_device_pixel_ratio();

    // 3. 计算物理像素尺寸 (比如 1600x1200)
    // 必须取整，否则可能导致渲染裂缝
    int phyW = (int)std::ceil(cssW * dpr);
    int phyH = (int)std::ceil(cssH * dpr);
    LF_LOGI("phyW=%d, phyH=%d, dpr=%f", phyW, phyH, dpr);

    // 4. 设置 Canvas 的“绘图缓冲区”尺寸
    // 这对应 HTML 标签的 <canvas width="..." height="...">
    emscripten_set_canvas_element_size(CANVAS_ID, phyW, phyH);

    // 5. 告诉你的引擎窗口变了
    // 注意：如果是 HiDPI，你可能需要根据你的 LFEngine 逻辑决定是传 cssW 还是 phyW
    // 如果你希望 100px 在 Web 上等于 100 CSS 像素（推荐），就传 cssW
    // 并在 Render 时应用 nvgScale(vg, dpr, dpr)
    // 这里为了简单，我们先传 CSS 尺寸，让 NanoVG 自动处理
    LFEngine::getInstance().setWindowSize((float)cssW, (float)cssH, (float)dpr);

    // 如果需要手动处理 DPR，可以在这里打印日志调试
    // printf("Resize: CSS=%.0fx%.0f, Phy=%dx%d, DPR=%.2f\n", cssW, cssH, phyW, phyH, dpr);
}

EM_BOOL on_resize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData) {
    update_canvas_size();
    return EM_TRUE;
}

void loop_callback() {
    LFEngine::getInstance().render();
}

int main() {
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = 0;
    attr.depth = 0;              // NanoVG 不需要深度缓冲
    attr.stencil = 1;            // <!!! 关键修复 !!!> NanoVG 必须要有 Stencil 缓冲
    attr.antialias = 1;          // 开启抗锯齿，文字更清晰
    attr.majorVersion = 2; // WebGL 2 => GLES 3
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(ctx);

    int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES;
    NVGcontext *vg = nvgCreateGLES3(flags);

    if (!vg) return 0;

    LFEngine::getInstance().init(vg);

    nvgCreateFont(vg, "sans", "fonts/MapleMonoNormalNL-Regular.ttf");

    LFResourceProvider::getInstance().setImageLoader(
            [](const std::string &path,
               std::function<void(std::shared_ptr<LFImageData>)> callback) {

                emscripten_fetch_attr_t attr;
                emscripten_fetch_attr_init(&attr);

                strcpy(attr.requestMethod, "GET");
                attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY; // 直接加载到内存

                // 关键点：我们需要把 callback 传给成功/失败的静态函数
                // 我们在堆上创建一个 callback 的拷贝，生命周期交给 Fetch 管理
                auto *callbackPtr = new std::function<void(std::shared_ptr<LFImageData>)>(callback);
                attr.userData = callbackPtr;

                // 成功回调
                attr.onsuccess = [](emscripten_fetch_t *fetch) {
                    auto *cb = static_cast<std::function<void(
                            std::shared_ptr<LFImageData>)> *>(fetch->userData);

                    auto img = std::make_shared<LFImageData>();
                    img->size = fetch->numBytes;
                    img->data = (unsigned char *) malloc(img->size);
                    memcpy(img->data, fetch->data, img->size);

                    // TODO: 依然需要解码宽高，见下文
                    img->width = 200;
                    img->height = 200;

                    LF_LOGI("Image Callback, Size=%zu", img->size);
                    (*cb)(img); // 调用引擎的回调

                    delete cb; // 清理 callback wrapper
                    emscripten_fetch_close(fetch); // 释放 fetch 资源
                };

                // 失败回调
                attr.onerror = [](emscripten_fetch_t *fetch) {
                    auto *cb = static_cast<std::function<void(
                            std::shared_ptr<LFImageData>)> *>(fetch->userData);
                    printf("Fetch failed: %s\n", fetch->url);

                    (*cb)(nullptr); // 告诉引擎加载失败

                    delete cb;
                    emscripten_fetch_close(fetch);
                };

                // 发起请求
                emscripten_fetch(&attr, path.c_str());
            }
    );

    auto root = build();
    LFEngine::getInstance().setRoot(root);

    update_canvas_size();
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_FALSE, on_resize);

    emscripten_set_main_loop(loop_callback, 0, 1);
}
