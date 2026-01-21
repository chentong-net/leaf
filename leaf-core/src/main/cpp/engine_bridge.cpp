#define NANOVG_GLES3_IMPLEMENTATION

#include "LFEngine.h"
#include "event/LFEvent.h"  // For LFPoint definition

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

void leaf_update_size(int w, int h, float d) {
    LFEngine::getInstance().setWindowSize((float)w, (float)h);
}

void leaf_render() {
    // Update logic (gesture timing, etc.)
    LFEngine::getInstance().update(0.016f);  // Assume 60fps (~16ms)

    // Render frame
    LFEngine::getInstance().render();
}

void leaf_eval_js(const char *code) {
    // ======================================
    // 手势系统完整测试 Demo
    // ======================================

    float fontSize = 42.0f;
    float rectWidth = 320.0f;
    float rectHeight = 160.0f;
    auto root = LFBox::create();
    root->matchParentWidth();
    root->matchParentHeight();
    root->setBackgroundColor(0xFF1A1A1A);  // 深色背景

    // 主内容区域 - 垂直滚动布局
    auto contentColumn = LFLinear::createVertical();
    contentColumn->matchParentWidth();
    contentColumn->wrapContentHeight();
    contentColumn->setGravity(LFAlignment::Start, LFAlignment::Center);
    contentColumn->setSpacing(40);
    contentColumn->setPadding(YGEdgeAll, 40);
    contentColumn->setMargin(YGEdgeTop, 40);

    // ====================
    // 1. Tap 手势测试
    // ====================
    auto tapBox = LFBox::create();
    tapBox->setWidth(rectWidth);
    tapBox->setHeight(rectHeight);
    tapBox->setBackgroundColor(0xFF4CAF50);
    tapBox->setBorderRadius(16.0f);
    tapBox->setShadow(0, 8, 16, 0, 0x40000000);

    auto tapText = createText("Tap", fontSize, 0xFFFFFFFF);
    tapText->setTextHAlign(LFTextHAlign::Center);
    tapText->setTextVAlign(LFTextVAlign::Center);
    tapText->setPadding(YGEdgeAll, 20);
    tapBox->addChild(tapText, LFBoxAlign::Center);

    // Tap 手势
    tapBox->setOnTap([tapBox](const LFPoint& location) {
        // 点击反馈: 改变颜色
        uint32_t color = tapBox->getBackgroundColor();
        if (color == 0xFF4CAF50) {
            tapBox->setBackgroundColor(0xFFFF0000);
        } else {
            tapBox->setBackgroundColor(0xFF4CAF50);
        }
    });

    // Double Tap 手势
    tapBox->setOnDoubleTap([tapBox](const LFPoint& location) {
        uint32_t color = tapBox->getBackgroundColor();
        if (color != 0xFF0000FF) {
            tapBox->setBackgroundColor(0xFF0000FF);
        } else {
            tapBox->setBackgroundColor(0xFF4CAF50);
        }
    });

    contentColumn->addChild(tapBox);

    // ====================
    // 2. Long Press 手势测试
    // ====================
    auto longPressBox = LFBox::create();
    longPressBox->setWidth(rectWidth);
    longPressBox->setHeight(rectHeight);
    longPressBox->setBackgroundColor(0xFFFF9800);
    longPressBox->setBorderRadius(16.0f);
    longPressBox->setShadow(0, 8, 16, 0, 0x40000000);

    auto longPressText = createText("Long Press", fontSize, 0xFFFFFFFF);
    longPressText->setTextHAlign(LFTextHAlign::Center);
    longPressText->setTextVAlign(LFTextVAlign::Center);
    longPressBox->addChild(longPressText, LFBoxAlign::Center);

    longPressBox->setOnLongPress([longPressBox](const LFPoint& location) {
        uint32_t color = longPressBox->getBackgroundColor();
        if (color == 0xFFFF9800) {
            longPressBox->setBackgroundColor(0xFF0000FF);
        } else {
            longPressBox->setBackgroundColor(0xFFFF9800);
        }
    });

    contentColumn->addChild(longPressBox);

    // ====================
    // 3. Pan (拖拽) 手势测试
    // ====================
    auto panBox = LFBox::create();
    panBox->setWidth(200.0f);
    panBox->setHeight(200.0f);
    panBox->setBackgroundColor(0xFF2196F3);
    panBox->setBorderRadius(100.0f);  // 圆形
    panBox->setShadow(0, 12, 24, 0, 0x60000000);

    auto panText = createText("Drag", fontSize, 0xFFFFFFFF);
    panText->setTextHAlign(LFTextHAlign::Center);
    panText->setTextVAlign(LFTextVAlign::Center);
    panBox->addChild(panText, LFBoxAlign::Center);

    // Pan 拖拽手势
    panBox->setOnPan(
        // onUpdate: 实时更新位置
        [panBox](const LFPoint& delta, const LFPoint& velocity) {
            auto transform = panBox->getTransform();
            panBox->setTranslate(
                transform.translateX + delta.x,
                transform.translateY + delta.y
            );
        },
        // onStart
        [](const LFPoint& delta, const LFPoint& velocity) {
        },
        // onEnd
        [](const LFPoint& delta, const LFPoint& velocity) {
        }
    );

    contentColumn->addChild(panBox);

    // ====================
    // 4. Pinch (缩放) 手势测试
    // ====================
    auto pinchBox = LFBox::create();
    pinchBox->setWidth(rectWidth);
    pinchBox->setHeight(rectWidth);
    pinchBox->setBackgroundColor(0xFFE91E63);
    pinchBox->setBorderRadius(20.0f);
    pinchBox->setShadow(0, 10, 20, 0, 0x50000000);

    auto pinchText = createText("Pinch", fontSize, 0xFFFFFFFF);
    pinchText->setTextHAlign(LFTextHAlign::Center);
    pinchText->setTextVAlign(LFTextVAlign::Center);
    pinchText->setLineHeight(1.5f);
    pinchBox->addChild(pinchText, LFBoxAlign::Center);

    // Pinch 缩放手势
    pinchBox->setOnPinch(
        // onUpdate: 应用缩放
        [pinchBox](float scale, const LFPoint& focal) {
            auto transform = pinchBox->getTransform();
            float newScaleX = transform.scaleX * scale;
            float newScaleY = transform.scaleY * scale;

            // 限制缩放范围 0.5x ~ 3.0x
            newScaleX = std::max(0.5f, std::min(3.0f, newScaleX));
            newScaleY = std::max(0.5f, std::min(3.0f, newScaleY));

            pinchBox->setScale(newScaleX, newScaleY);
        },
        // onStart
        [](float scale, const LFPoint& focal) {
        },
        // onEnd
        [](float scale, const LFPoint& focal) {
        }
    );

    contentColumn->addChild(pinchBox);

    // ====================
    // 5. Rotate (旋转) 手势测试
    // ====================
    auto rotateBox = LFBox::create();
    rotateBox->setWidth(rectWidth);
    rotateBox->setHeight(rectWidth);
    rotateBox->setBackgroundColor(0xFF9C27B0);
    rotateBox->setBorderRadius(24.0f);
    rotateBox->setShadow(0, 10, 20, 0, 0x50000000);

    auto rotateText = createText("Rotate", fontSize, 0xFFFFFFFF);
    rotateText->setTextHAlign(LFTextHAlign::Center);
    rotateText->setTextVAlign(LFTextVAlign::Center);
    rotateText->setLineHeight(1.5f);
    rotateBox->addChild(rotateText, LFBoxAlign::Center);

    // Rotate 旋转手势
    rotateBox->setOnRotate(
        // onUpdate: 应用旋转
        [rotateBox](float angle, const LFPoint& focal) {
            auto transform = rotateBox->getTransform();
            float degrees = angle * 180.0f / M_PI;  // 弧度转角度
            rotateBox->setRotate(transform.rotate + degrees);
        },
        // onStart
        [](float angle, const LFPoint& focal) {
        },
        // onEnd
        [](float angle, const LFPoint& focal) {
        }
    );

    contentColumn->addChild(rotateBox);

    // ====================
    // 6. Swipe (轻扫) 手势测试
    // ====================
    auto swipeContainer = LFLinear::createHorizontal();
    swipeContainer->setWidthPercent(90.0f);
    swipeContainer->wrapContentHeight();
    swipeContainer->setDistribution(LFDistribution::SpaceBetween);

    // 创建4个方向的swipe测试框
    auto createSwipeBox = [](const std::string& label, int allowedDir, float fontSize, uint32_t color) {
        auto box = LFBox::create();
        box->setWidth(320.0f);
        box->setHeight(160.0f);
        box->setBackgroundColor(color);
        box->setBorderRadius(12.0f);
        box->setShadow(0, 6, 12, 0, 0x40000000);

        auto text = createText(label, fontSize, 0xFFFFFFFF);
        text->setTextHAlign(LFTextHAlign::Center);
        text->setTextVAlign(LFTextVAlign::Center);
        text->setPadding(YGEdgeAll, 20);  // Add padding to text node itself
        box->addChild(text, LFBoxAlign::Center);

        // Swipe 手势 (只允许特定方向)
        box->setOnSwipe(
            [box, label](int direction, const LFPoint& velocity) {
                const char* dirName = "";
                if (direction == 1) dirName = "LEFT";
                else if (direction == 2) dirName = "RIGHT";
                else if (direction == 4) dirName = "UP";
                else if (direction == 8) dirName = "DOWN";

                // 视觉反馈
                box->setOpacity(0.6f);
            },
            allowedDir  // 方向过滤
        );

        return box;
    };

    // 左右滑动测试
    auto swipeHorizontal = createSwipeBox("Swipe ←→", 1 | 2, fontSize, 0xFF00BCD4);  // Left | Right
    contentColumn->addChild(swipeHorizontal);

    // 上下滑动测试
    auto swipeVertical = createSwipeBox("Swipe ↑↓", 4 | 8, fontSize, 0xFF009688);    // Up | Down
    contentColumn->addChild(swipeVertical);

    // ====================
    // 7. 手势竞争测试: Tap vs Pan
    // ====================
    auto competitionBox = LFBox::create();
    competitionBox->setWidthPercent(85.0f);
    competitionBox->setHeight(rectHeight);
    competitionBox->setBackgroundColor(0xFFFF5722);
    competitionBox->setBorderRadius(16.0f);
    competitionBox->setShadow(0, 8, 16, 0, 0x40000000);
    competitionBox->setBorder(4.0f, 0xFFFFFFFF);

    auto competitionText = createText("TAP or PAN\n(Arena Test)", fontSize, 0xFFFFFFFF);
    competitionText->setTextHAlign(LFTextHAlign::Center);
    competitionText->setTextVAlign(LFTextVAlign::Center);
    competitionText->setLineHeight(1.4f);
    competitionText->setPadding(YGEdgeAll, 20);  // Add padding to text node itself
    competitionBox->addChild(competitionText, LFBoxAlign::Center);

    // 同时添加 Tap 和 Pan, 测试竞技场
    competitionBox->setOnTap([](const LFPoint& location) {
    });

    competitionBox->setOnPan(
        [competitionBox](const LFPoint& delta, const LFPoint& velocity) {
            auto transform = competitionBox->getTransform();
            competitionBox->setTranslate(
                transform.translateX + delta.x,
                transform.translateY + delta.y
            );
        },
        [](const LFPoint& delta, const LFPoint& velocity) {
        }
    );

    contentColumn->addChild(competitionBox);

    root->addChild(contentColumn, LFBoxAlign::TopLeft);

    // 提交给引擎
    LFEngine::getInstance().setRoot(root);
}

}