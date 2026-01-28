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
#include "LFButton.h"

static const char* CANVAS_ID = "#canvas";
double dpr = 1.0f;

std::shared_ptr<LFText> createText(const std::string& content, float fontSize, uint32_t color, bool isBold = false) {
    auto text = std::make_shared<LFText>();
    text->setText(content);
    text->setFontSize(fontSize);
    text->setTextColor(color);
    return text;
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


// 获取当前时间戳
double get_engine_time() {
    return LFEngine::getInstance().getElapsedTime();
}

// 统一的事件分发入口
void dispatch_to_engine(LFTouchEventType type, const std::vector<LFTouchPoint>& touches, const std::vector<LFTouchID>& changedIds) {
    auto root = LFEngine::getInstance().getRoot();
    if (root) {
        LFEventDispatcher::getInstance().dispatchTouchEvent(type, touches, changedIds, root);
    }
}

// ==========================================
// 鼠标事件适配 (模拟单指触摸)
// ==========================================
EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *userData) {
    // 只处理左键 (button 0)
    if (e->button != 0) return EM_FALSE;

    LFTouchEventType type;
    if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN) type = LFTouchEventType::Down;
    else if (eventType == EMSCRIPTEN_EVENT_MOUSEMOVE) {
        // 只有按住左键移动时才算 Drag/Move
        if (e->buttons & 1) type = LFTouchEventType::Move;
        else return EM_FALSE; // 只是 hover，移动端引擎通常忽略
    }
    else if (eventType == EMSCRIPTEN_EVENT_MOUSEUP) type = LFTouchEventType::Up;
    else return EM_FALSE;

    // 构造 TouchPoint
    LFTouchPoint p;
    p.id = 0; // 鼠标固定 ID 为 0
    p.x = e->targetX;
    p.y = e->targetY;
    p.pressure = 1.0f;
    p.timestamp = get_engine_time();

    std::vector<LFTouchPoint> touches = { p };
    std::vector<LFTouchID> changed = { 0 };

    dispatch_to_engine(type, touches, changed);

    return EM_TRUE; // 消费事件
}

// ==========================================
// 触摸事件适配 (原生多点触控)
// ==========================================
EM_BOOL touch_callback(int eventType, const EmscriptenTouchEvent *e, void *userData) {
    LFTouchEventType type;
    if (eventType == EMSCRIPTEN_EVENT_TOUCHSTART) type = LFTouchEventType::Down;
    else if (eventType == EMSCRIPTEN_EVENT_TOUCHMOVE) type = LFTouchEventType::Move;
    else if (eventType == EMSCRIPTEN_EVENT_TOUCHEND) type = LFTouchEventType::Up;
    else if (eventType == EMSCRIPTEN_EVENT_TOUCHCANCEL) type = LFTouchEventType::Cancel;
    else return EM_FALSE;

    std::vector<LFTouchPoint> allTouches;
    std::vector<LFTouchID> changedIds;

    double now = get_engine_time();

    // 1. 收集当前屏幕上所有的点 (Active Touches)
    for (int i = 0; i < e->numTouches; ++i) {
        if (!e->touches[i].isChanged) {
            // 这是一个在屏幕上但状态没变的点，也需要传给引擎保持状态
            LFTouchPoint p;
            p.id = (int)e->touches[i].identifier;
            p.x = e->touches[i].targetX;
            p.y = e->touches[i].targetY;
            p.pressure = 1.0f;
            p.timestamp = now;
            allTouches.push_back(p);
        }
    }

    // 2. 收集发生变化的点 (Changed Touches)
    // Emscripten 的结构略有不同，changedTouches 是单独的列表吗？
    // 注意：Emscripten 这里的结构包含 touches (所有点) 和 changedTouches (变化点)
    // 但 C API 中 `EmscriptenTouchEvent` 结构体直接给出了 `touches` 数组，
    // 我们需要通过 `e->touches[i].isChanged` 标志来判断。

    for (int i = 0; i < e->numTouches; ++i) {
        if (e->touches[i].isChanged) {
            LFTouchPoint p;
            p.id = (int)e->touches[i].identifier;
            p.x = e->touches[i].targetX;
            p.y = e->touches[i].targetY;
            p.pressure = 1.0f;
            p.timestamp = now;

            allTouches.push_back(p);
            changedIds.push_back(p.id);
        }
    }

    dispatch_to_engine(type, allTouches, changedIds);

    // 返回 EM_TRUE 非常重要！这会调用 e.preventDefault()
    // 防止浏览器处理滑动（比如页面滚动、下拉刷新等）
    return EM_TRUE;
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

    // 4. 设置 Canvas 的“绘图缓冲区”尺寸
    // 这对应 HTML 标签的 <canvas width="..." height="...">
    emscripten_set_canvas_element_size(CANVAS_ID, phyW, phyH);

    // 5. 告诉你的引擎窗口变了
    // 注意：如果是 HiDPI，你可能需要根据你的 LFEngine 逻辑决定是传 cssW 还是 phyW
    // 如果你希望 100px 在 Web 上等于 100 CSS 像素（推荐），就传 cssW
    // 并在 Render 时应用 nvgScale(vg, dpr, dpr)
    // TODO: 这里为了简单，我们先传 CSS 尺寸，让 NanoVG 自动处理
    LFEngine::getInstance().setWindowSize((float)cssW, (float)cssH, (float)dpr);

    // 如果需要手动处理 DPR，可以在这里打印日志调试
    // printf("Resize: CSS=%.0fx%.0f, Phy=%dx%d, DPR=%.2f\n", cssW, cssH, phyW, phyH, dpr);
}

EM_BOOL on_resize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData) {
    update_canvas_size();
    return EM_TRUE;
}

void loop_callback() {
    LFEngine::getInstance().update(0.016f);
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

    auto navigator = LFNavigator::create();
    auto rootPage = LFPage::create();
    rootPage->setBackgroundColor(0xFFF5F5F5);
    auto root = buildRootNode(navigator, buildTouchNode());
    rootPage->addChild(root);
    navigator->push(rootPage);
    LFEngine::getInstance().setRoot(navigator);


    emscripten_set_mousedown_callback(CANVAS_ID, nullptr, EM_TRUE, mouse_callback);
    emscripten_set_mouseup_callback(CANVAS_ID, nullptr, EM_TRUE, mouse_callback);
    emscripten_set_mousemove_callback(CANVAS_ID, nullptr, EM_TRUE, mouse_callback);

    emscripten_set_touchstart_callback(CANVAS_ID, nullptr, EM_TRUE, touch_callback);
    emscripten_set_touchend_callback(CANVAS_ID, nullptr, EM_TRUE, touch_callback);
    emscripten_set_touchmove_callback(CANVAS_ID, nullptr, EM_TRUE, touch_callback);
    emscripten_set_touchcancel_callback(CANVAS_ID, nullptr, EM_TRUE, touch_callback);

    update_canvas_size();
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_FALSE, on_resize);

    emscripten_set_main_loop(loop_callback, 0, 1);
}
