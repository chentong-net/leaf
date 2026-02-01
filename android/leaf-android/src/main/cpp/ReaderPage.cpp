//
// Created by Chen Tong on 2026/1/31.
//

#include "ReaderPage.h"
#include "LFEngine.h"
#include "LFButton.h"
#include "LFText.h"
#include "LFNavigator.h"
#include "LFGlobalAnimationManager.h"
#include "TextSplitter.h" // 引入业务层的工具

#include <sstream>

static std::string normalizeText(const std::string& input) {
    if (input.empty()) return "";

    std::string output;
    output.reserve(input.size());

    // 1. 遍历清洗
    for (char c : input) {
        // 彻底丢弃 \r (Carriage Return)，它是导致行高计算翻倍的元凶
        if (c == '\r') {
            continue;
        }
        output.push_back(c);
    }

    // 2. (可选) 处理连续的空行？
    // 如果想要紧凑排版，可以将连续的 \n\n\n 压缩为 \n\n。
    // 但考虑到小说需要空行表现节奏，这里我们只去 \r，保留 \n。

    return output;
}

// ==========================================
// Internal Adapter Implementation
// ==========================================

/**
 * 书籍页面适配器
 * 负责将 TextSplitter 切好的 string 数据绑定到 View 上
 */
class BookPageAdapter : public LFPageAdapter {
public:
    BookPageAdapter(const std::vector<std::string>& pages, float fontSize, float topPadding)
            : m_pages(pages)
            , m_fontSize(fontSize)
            , m_topPadding(topPadding)
    {}

    int getCount() override {
        return (int)m_pages.size();
    }

    LFNode::Ptr createView() override {
        // 1. 根容器 (LFBox)
        // 我们需要一个 Box 来包裹 "正文" 和 "页码"
        auto container = LFBox::create();
        container->matchParentWidth();
        container->matchParentHeight();
        // 容器本身透明，背景色由 ReaderPage 统一设置

        // --- 正文区域 ---
        auto contentText = std::make_shared<LFText>();
        contentText->setFontSize(m_fontSize);
        contentText->setLineHeight(1.6f);      // 舒适行高
        contentText->setTextColor(0xFF333333); // 深灰字 (#333333)
        contentText->setTextHAlign(LFTextHAlign::Left);
        contentText->setTextVAlign(LFTextVAlign::Top);

        // 布局设置：撑满宽度
        contentText->matchParentWidth();
        contentText->matchParentHeight();

        // 关键：内边距设置
        // Top: 避开顶部标题栏
        // Bottom: 避开底部页码
        // Horizontal: 阅读舒适区
        contentText->setPadding(YGEdgeTop, m_topPadding + 20.0f);
        contentText->setPadding(YGEdgeBottom, 50.0f);
        contentText->setPadding(YGEdgeHorizontal, 24.0f);

        // --- 页码区域 (Footer) ---
        auto footerText = std::make_shared<LFText>();
        footerText->setFontSize(11.0f);
        footerText->setTextColor(0xFFAAAAAA); // 浅灰 (#AAAAAA)
        footerText->setTextHAlign(LFTextHAlign::Center);
        footerText->setTextVAlign(LFTextVAlign::Center);

        // 将子 View 加入容器
        // Index 0: 正文
        container->addChild(contentText, LFBoxAlign::MatchParent);
        // Index 1: 页码 (使用绝对定位放在底部)
        container->addChild(footerText, LFBoxAlign::BottomCenter, 0, -20.0f);

        return container;
    }

    void bindView(LFNode::Ptr view, int index) override {
        auto container = std::static_pointer_cast<LFBox>(view);
        auto children = container->getChildren();
        if (children.size() < 2) return;

        // 1. 绑定正文
        auto contentText = std::static_pointer_cast<LFText>(children[0]);
        if (index >= 0 && index < m_pages.size()) {
            contentText->setText(m_pages[index]);
        } else {
            contentText->setText(""); // 安全清空
        }

        // 2. 绑定页码 "1 / 150"
        auto footerText = std::static_pointer_cast<LFText>(children[1]);
        std::stringstream ss;
        ss << (index + 1) << " / " << m_pages.size();
        footerText->setText(ss.str());
    }

private:
    std::vector<std::string> m_pages;
    float m_fontSize;
    float m_topPadding;
};

// ==========================================
// ReaderPage Implementation
// ==========================================

ReaderPage::Ptr ReaderPage::create(const std::string& bookTitle, const std::string& content) {
    auto page = std::make_shared<ReaderPage>();
    page->initLayout(bookTitle, content);
    return page;
}

ReaderPage::ReaderPage() {
    // 设置经典的护眼背景色 (米黄/羊皮纸色)
    setBackgroundColor(0xFFF6F1E1);
}

void ReaderPage::initLayout(const std::string& title, const std::string& content) {
    // 根节点：Box 布局 (用于层叠 PageView 和 TopBar)
    auto root = LFBox::create();
    root->matchParentWidth();
    root->matchParentHeight();

    // 1. 创建 PageView (核心阅读器)
    m_pageView = LFPageView::create();
    m_pageView->matchParentWidth();
    m_pageView->matchParentHeight();

    // 2. 添加点击事件 (唤出/隐藏菜单)
    // 这里的 setOnTap 是 LFNode 的方法。
    // 手势竞技场会自动处理：如果是滑动，PageView 消耗 Pan 事件；如果是点击，这里触发 Tap。
    std::weak_ptr<ReaderPage> weakSelf = std::static_pointer_cast<ReaderPage>(shared_from_this());
    m_pageView->setOnTap([weakSelf](const LFPoint& p) {
        if (auto self = weakSelf.lock()) {
            self->toggleMenu();
        }
    });

    root->addChild(m_pageView, LFBoxAlign::MatchParent);

    // 3. 设置 TopBar (UI 放在上面)
    setupTopBar(title);
    root->addChild(m_topBar, LFBoxAlign::TopLeft);

    addChild(root);

    // 4. 数据处理 (同步或异步)
    splitContentAndInit(content);
}

void ReaderPage::splitContentAndInit(const std::string& rawContent) {
    std::string content = normalizeText(rawContent);
    // ==========================================
    // 1. 布局常量定义 (The Source of Truth)
    // ==========================================
    // 必须与 BookPageAdapter::createView 中的设置严格一致
    // ------------------------------------------
    const float fontSize = 18.0f;
    const float lineHeightScale = 1.6f;

    // 垂直方向 (Pixels)
    const float topBarHeight = 56.0f;       // 顶部导航栏高度
    const float adapterPaddingTop = 20.0f;  // 正文顶部内边距 (Adapter setPadding Top)
    const float adapterPaddingBottom = 50.0f; // 正文底部内边距 (Adapter setPadding Bottom)
    const float footerReserve = 30.0f;      // 底部页码区域预留高度

    // 水平方向 (Pixels)
    const float adapterPaddingX = 24.0f;    // 左右内边距 (Adapter setPadding Horizontal)

    // ==========================================
    // 2. 获取屏幕尺寸
    // ==========================================
    float windowW = LFEngine::getInstance().getWindowWidth();
    float windowH = LFEngine::getInstance().getWindowHeight();

    // ==========================================
    // 3. 计算安全宽度 (Width Safety)
    // ==========================================
    // 逻辑：屏幕宽 - (左边距 + 右边距) - 宽度公差
    // [宽度公差 5.0f]: 防止 Splitter 认为刚好能排满一行，而渲染器因为精度误差挤到下一行
    const float widthTolerance = 5.0f;
    float safeWidth = windowW - (adapterPaddingX * 2.0f) - widthTolerance;

    if (safeWidth <= 0) safeWidth = 100; // 防御性保护

    // ==========================================
    // 4. 计算安全高度 (Height Safety)
    // ==========================================
    // 逻辑：屏幕高 - 顶部占用 - 底部占用 - 高度缓冲

    // 固定被占用的高度
    float occupiedHeight = topBarHeight + adapterPaddingTop + adapterPaddingBottom;

    // 既然 TextSplitter 现在计算很精准，我们只需要极小的缓冲来防止 float 精度误差
    float singleLineHeight = fontSize * lineHeightScale;
    float safetyBuffer = 5.0f;

    float safeHeight = windowH - occupiedHeight - safetyBuffer;

    // 防御性检查
    if (safeHeight < singleLineHeight) safeHeight = singleLineHeight;

    // ==========================================
    // 5. 配置与执行切分
    // ==========================================
    SplitConfig config;
    config.width = safeWidth;
    config.height = safeHeight; // 传给 Splitter 一个更矮的高度限制
    config.fontSize = fontSize;
    config.lineHeight = lineHeightScale;
    config.fontFamily = "sans"; // 确保和 Adapter 字体一致

    LF_LOGI("Layout Logic: Win=%.0fx%.0f, Safe=%.0fx%.0f (Buffer=%.1f)",
            windowW, windowH, safeWidth, safeHeight, safetyBuffer);

    // 调用业务工具切分 (耗时操作，建议在正式版中放入线程池)
    auto pages = TextSplitter::split(content, config);

    LF_LOGI("Split Result: %zu pages generated.", pages.size());

    // ==========================================
    // 6. 设置适配器
    // ==========================================
    if (!pages.empty()) {
        // 创建适配器，传入 topBarHeight 以便 Adapter 内部设置正确的 Padding
        auto adapter = std::make_shared<BookPageAdapter>(pages, fontSize, topBarHeight);
        m_pageView->setAdapter(adapter);
    }
}

void ReaderPage::setupTopBar(const std::string& title) {
    m_topBar = LFLinear::createHorizontal();
    m_topBar->matchParentWidth();
    m_topBar->setHeight(56.0f);
    m_topBar->setBackgroundColor(0xFFFFFFFF); // 白底
    m_topBar->setGravity(LFAlignment::Start, LFAlignment::Center);
    m_topBar->setPadding(YGEdgeHorizontal, 10.0f);

    // 增加一点阴影让层级更明显
    m_topBar->setShadow(0, 2, 8, 0, 0x1A000000);

    std::weak_ptr<ReaderPage> weakSelf = std::static_pointer_cast<ReaderPage>(shared_from_this());

    // 返回按钮
    auto backBtn = LFButton::create();
    backBtn->setWidth(44);
    backBtn->setHeight(44);

    auto backText = std::make_shared<LFText>();
    backText->setText("<"); // 或使用图标
    backText->setFontSize(24);
    backText->setTextColor(0xFF333333);
    backBtn->addChild(backText, LFBoxAlign::Center);

    backBtn->setOnClick([weakSelf](LFButton* btn){
        if (auto self = weakSelf.lock()) {
            if (auto nav = self->getNavigator()) {
                nav->pop();
            }
        }
    });

    // 标题
    auto titleText = std::make_shared<LFText>();
    titleText->setText(title);
    titleText->setFontSize(16);
    titleText->setTextColor(0xFF333333);
    // 限制标题长度，超出... (需要 Text 支持，或者简单截断)

    m_topBar->addChild(backBtn);
    m_topBar->addChild(titleText);

    // 初始状态：显示
    m_isMenuVisible = true;
}

void ReaderPage::toggleMenu() {
    m_isMenuVisible = !m_isMenuVisible;

    // 简单的动画效果：改变透明度或位置
    if (m_topBar) {
        // 使用 LFValueAnimator 做一个简单的滑出/滑入动画会更棒
        // 这里为了简单演示，直接 toggle visibility
        // m_topBar->setVisible(m_isMenuVisible);

        // 进阶：使用 Translation 动画
        float startY = m_topBar->getTranslateY();
        float endY = m_isMenuVisible ? 0.0f : -60.0f; // 向上收起

        auto anim = LFValueAnimator<float>::of(startY, endY);
        anim->setDuration(0.2f);
        anim->setEasing(LFEasingType::QuadOut);

        // 捕获 strong ptr 因为 topBar 是成员变量
        auto topBar = m_topBar;
        anim->addUpdateListener([topBar](float val) {
            topBar->setTranslate(0, val);
        });
        anim->start();
        LFGlobalAnimationManager::getInstance().addAnimator(anim);
    }
}