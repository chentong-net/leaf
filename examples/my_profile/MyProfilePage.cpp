//
// Created by Chen Tong on 2026/2/2.
//

#include "MyProfilePage.h"
#include "LFImage.h"
#include "LFText.h"
#include "LFButton.h"
#include "LFNavigator.h"

std::shared_ptr<LFNode> buildICP();

static std::shared_ptr<LFText> createLabel(const std::string& text, float size, uint32_t color, bool bold = false) {
    auto t = std::make_shared<LFText>();
    t->setText(text);
    t->setFontSize(size);
    t->setTextColor(color);
    t->setTextHAlign(LFTextHAlign::Center);
    t->setTextVAlign(LFTextVAlign::Center);
    // TODO: 后续支持 setFontFamily(bold)，可以在这里添加逻辑
    return t;
}

// --- 颜色配置 (Color Palette) ---
static const uint32_t COL_BG          = 0xFFF5F7FA; // 页面背景
static const uint32_t COL_CARD_BG     = 0xFFFFFFFF; // 卡片背景
static const uint32_t COL_PRIMARY     = 0xFF0052D9; // 科技蓝
static const uint32_t COL_TEXT_MAIN   = 0xFF1D1D1F; // 主要文字
static const uint32_t COL_TEXT_SUB    = 0xFF6E6E73; // 次要文字
static const uint32_t COL_DIVIDER     = 0xFFE5E5E5; // 分割线
static const uint32_t COL_SHADOW      = 0x1A000000; // 阴影颜色 (10%透明度)

// --- 字体大小 ---
static const float FS_TITLE  = 22.0f;
static const float FS_HEAD   = 18.0f;
static const float FS_BODY   = 15.0f;
static const float FS_SMALL  = 13.0f;

std::shared_ptr<MyProfilePage> MyProfilePage::create() {
    auto page = std::make_shared<MyProfilePage>();
    page->initUI();
    return page;
}

MyProfilePage::MyProfilePage() {
    // 构造函数
}

void MyProfilePage::onEnter() {
    LFPage::onEnter();
    // 可以在这里做埋点统计
}

void MyProfilePage::initUI() {
    setBackgroundColor(COL_BG);

    // 1. 全局滚动容器
    auto scrollView = LFScrollView::createVertical();
    scrollView->matchParentWidth();
    scrollView->matchParentHeight();
    scrollView->setScrollBarEnabled(false);
    scrollView->setBounces(false);
    addChild(scrollView);

    // 2. 垂直线性布局容器
    auto content = LFLinear::createVertical();
    content->matchParentWidth();
    content->wrapContentHeight();
    content->setPadding(YGEdgeAll, PAGE_PADDING); // 全局内边距
    // 增加底部留白，防止内容贴底
    content->setPadding(YGEdgeBottom, 20.0f);

    // ==========================================
    // 组装各板块
    // ==========================================

    // A. 头部信息
    content->addChild(createHeaderSection());

    // B. 核心指标栏 (年龄/学历等)
    content->addChild(createStatsBar());

    // C. 专业技能
    content->addChild(createSectionTitle("专业技能"));
    content->addChild(createSkillCloud());

    // D. 工作经历
    content->addChild(createSectionTitle("工作经历"));
    // 这种容器用于绘制左侧的时间轴线
    auto workContainer = LFLinear::createVertical();
    workContainer->matchParentWidth();
    workContainer->addChild(createWorkExperienceItem());
    content->addChild(workContainer);

    // E. 教育背景
    content->addChild(createSectionTitle("教育背景"));
    auto eduContainer = LFLinear::createVertical();
    eduContainer->matchParentWidth();
    eduContainer->setSpacing(16);
    eduContainer->addChild(createEducationCard(
            "",
            "",
            "",
            "",
            "",
            false
    ));
    eduContainer->addChild(createEducationCard(
            "",
            "",
            "",
            "",
            "",
            true
    ));
    eduContainer->setMargin(YGEdgeBottom, 16);
    content->addChild(eduContainer);

    // F. 项目经历 (重点)
    content->addChild(createSectionTitle("项目经历"));
    content->addChild(createProjectCard(
            "",
            "",
            "",
            ""
    ));
    content->addChild(createProjectCard(
            "",
            "",
            "",
            // 传递给详情页的内容：
            ""
    ));
    content->addChild(createProjectCard(
            "",
            "",
            "",
            ""
    ));
    content->addChild(createProjectCard(
            "",
            "",
            "",
            ""
    ));

    content->addChild(buildICP());

    // 将内容容器放入滚动视图
    // 注意：ScrollView 的 addChild 其实是加到内部的 content 中
    // 但这里我们是手动创建了一个 content 塞进去，这取决于 ScrollView 的实现。
    // 根据之前的代码，LFScrollView::createVertical() 内部已经有一个 content 了。
    // 所以正确的做法是：直接添加到 scrollView

    // *修正*：根据之前的 LFScrollView 实现，addChild 是代理到内部 linear 的。
    // 所以我们应该直接把上面这些 child 加到 scrollView 里？
    // 不，为了控制 Padding 和整体布局，最好还是把 content 加到 scrollView。
    // 我们需要清空 scrollView 默认的 content 或者将 content 下挂。
    // 假设 LFScrollView::addChild 是加到内部 Linear，那我们上面创建的 content 应该作为唯一的 child 加入。
    scrollView->addChild(content);
}

// ==========================================
// 组件实现细节
// ==========================================

std::shared_ptr<LFNode> MyProfilePage::createHeaderSection() {
    auto card = LFLinear::createVertical();
    card->matchParentWidth();
    card->setHeight(200);
//    card->setBackgroundColor(COL_CARD_BG);
//    card->setBorderRadius(CARD_RADIUS);
//    card->setShadow(0, 4, 12, 0, COL_SHADOW);
    card->setMargin(YGEdgeBottom, 16);
    card->setGravity(LFAlignment::Center, LFAlignment::Center);

    // 使用水平布局放 头像 + 信息
    auto column = LFLinear::createVertical();
    column->setSpacing(10);
    column->matchParentWidth();
    column->matchParentHeight();
    column->setAlignItems(YGAlignCenter); // 垂直居中
    column->setGravity(LFAlignment::Center, LFAlignment::Center);

    // 1. 头像
    auto avatar = std::make_shared<LFImage>();
    avatar->setSrc("");
    avatar->setWidth(80);
    avatar->setHeight(80);
    avatar->setBorderRadius(40); // 圆形
    avatar->setBackgroundColor(0xFFE0E0E0); // 占位色
    avatar->setBorder(2.0f, 0xFFFFFFFF); // 白圈描边，增加精致感
    avatar->setShadow(0, 2, 8, 0, 0x33000000);
    column->addChild(avatar);

    auto name = createLabel("", 26, COL_TEXT_MAIN, true);
    name->setTextHAlign(LFTextHAlign::Center);
    name->setMargin(YGEdgeTop, 20);
    column->addChild(name);

    auto role = createLabel("", 15, COL_TEXT_SUB);
    role->setTextHAlign(LFTextHAlign::Center);
    role->setMargin(YGEdgeTop, 5);
    column->addChild(role);

    auto contact = std::make_shared<LFText>();
    contact->wrapContentHeight();
    contact->setTextHAlign(LFTextHAlign::Center);
    contact->setTextVAlign(LFTextVAlign::Center);
    contact->setText("");
    contact->setTextColor(COL_PRIMARY);
    contact->setFontSize(11);

    column->addChild(contact);
    card->addChild(column); // Box 需要指定 Align

    return card;
}

std::shared_ptr<LFNode> MyProfilePage::createStatsBar() {
    auto container = LFLinear::createHorizontal();
    container->matchParentWidth();
    container->setDistribution(LFDistribution::SpaceAround); // 平均分布
    container->setPadding(YGEdgeVertical, 10);
    container->setMargin(YGEdgeBottom, 10);

    // 辅助：创建单个统计项
    auto makeStat = [](const std::string& val, const std::string& label) {
        auto col = LFLinear::createVertical();
        col->setAlignItems(YGAlignCenter);

        auto v = createLabel(val, 18, COL_TEXT_MAIN, true);
        col->addChild(v);

        auto l = createLabel(label, 12, COL_TEXT_SUB);
        l->setMargin(YGEdgeTop, 4);
        col->addChild(l);
        return col;
    };

    container->addChild(makeStat("", "工作经验"));
    container->addChild(makeStat("", "年龄"));
    container->addChild(makeStat("", "学历"));
    container->addChild(makeStat("", "政治面貌"));

    return container;
}

std::shared_ptr<LFNode> MyProfilePage::createSectionTitle(const std::string& title) {
    auto text = createLabel(title, FS_HEAD, COL_TEXT_MAIN, true);
    text->setTextHAlign(LFTextHAlign::Left);
    text->setMargin(YGEdgeTop, 15);
    text->setMargin(YGEdgeBottom, 12);
    // 左侧加一个小竖条装饰，增加设计感
    auto wrapper = LFLinear::createHorizontal();
    wrapper->setGravity(LFAlignment::Start, LFAlignment::Center);

    auto bar = LFBox::create();
    bar->setWidth(4);
    bar->setHeight(16);
    bar->setBackgroundColor(COL_PRIMARY);
    bar->setBorderRadius(2);
    bar->setMargin(YGEdgeRight, 8);

    wrapper->addChild(bar);
    wrapper->addChild(text);
    return wrapper;
}

std::shared_ptr<LFNode> MyProfilePage::createSkillCloud() {
    auto card = LFLinear::createVertical();
    card->matchParentWidth();
    card->wrapContentHeight();
    // card->setBackgroundColor(0x80FFFFFF);
    // card->setBorderRadius(CARD_RADIUS);
    // card->setShadow(0, 2, 8, 0, 0x0D000000); // 极淡阴影
    card->setPadding(YGEdgeHorizontal, 16);
    card->setPadding(YGEdgeBottom, 16);

    auto flow = LFLinear::createHorizontal();
    flow->matchParentWidth();
    flow->wrapContentHeight();
    flow->setSpacing(10);
    flow->setGravity(LFAlignment::Start, LFAlignment::Center);
    flow->setFlexWrap(YGWrapWrap); // 关键：自动换行

    struct Skill {
        std::string name;
        uint32_t bgColor;
        uint32_t pressedColor;
        uint32_t textColor;
    };

    std::vector<Skill> skills = {
            {"Android", 0xFFE8F5E9, 0xFFC8E6C9, 0xFF2E7D32},
            {"iOS", 0xFFF5F5F5, 0xFFEEEEEE, 0xFF424242},
            {"Flutter", 0xFFE1F5FE, 0xFFB3E5FC, 0xFF0277BD},
            {"OpenGL", 0xFFF3E5F5, 0xFFE1BEE7, 0xFF7B1FA2},
            {"FFmpeg", 0xFFF1F8E9, 0xFFDCEDC8, 0xFF33691E},
            {"Server", 0xFFE8EAF6, 0xFFC5CAE9, 0xFF3F51B5},
            {"Golang", 0xFFE0F7FA, 0xFFB2EBF2, 0xFF00838F},
    };

    for (const auto& skill : skills) {
        auto btn = LFButton::create();
        btn->setWidth(60);
        btn->setHeight(30);
        btn->setBackgroundColor(LFButtonState::Normal, skill.bgColor);
        btn->setBackgroundColor(LFButtonState::Pressed, skill.pressedColor);
        btn->setBorderRadius(15);
        btn->setText(skill.name);
        btn->setFontSize(11);
        btn->setTextColor(skill.textColor);
        btn->setOnTap([skill](const LFPoint& location) {
            LF_LOGI("%s", skill.name.c_str());
        });
        flow->addChild(btn);
    }
    card->addChild(flow);
    return card;
}

std::shared_ptr<LFNode> MyProfilePage::createWorkExperienceItem()
{
    std::string company = "";
    std::string role = "";
    std::string time = "";
//    std::string desc = "";

    auto row = LFLinear::createHorizontal();
    row->matchParentWidth();
    row->setHeight(80);
    row->setSpacing(10);
    row->setGravity(LFAlignment::Start, LFAlignment::Center);
    row->setPadding(YGEdgeHorizontal, 16);
    row->setBackgroundColor(COL_CARD_BG);
    row->setBorderRadius(CARD_RADIUS);
    row->setShadow(0, 4, 12, 0, COL_SHADOW);

//    auto logoContainer = LFLinear::createVertical();
//    logoContainer->setGravity(LFAlignment::Start, LFAlignment::Start);
    auto logo = std::make_shared<LFImage>();
    logo->setFit(LFImageFit::Fill);
    logo->setSrc("");
    logo->setWidth(50);
    logo->setHeight(50);
    logo->setBorderRadius(CARD_RADIUS);
    logo->setBorder(1, 0xFFD1E1FA);
    row->addChild(logo);

//    // --- 左侧：时间轴装饰 ---
//    auto timeline = LFLinear::createVertical();
//    timeline->setWidth(24);
//    timeline->setAlignItems(YGAlignCenter);
//    timeline->setMargin(YGEdgeRight, 10);
//
//    // 圆点
//    auto dot = LFBox::create();
//    dot->setWidth(10);
//    dot->setHeight(10);
//    dot->setBorderRadius(5);
//    dot->setBackgroundColor(COL_PRIMARY);
//    // 外圈光晕
//    dot->setBorder(2, 0xFFD1E1FA); // 淡蓝光晕
//    timeline->addChild(dot);
//
//    // 竖线
//    if (!isLast) {
//        auto line = LFBox::create();
//        line->setWidth(2);
//        line->setFlexGrow(1.0f);
//        line->setBackgroundColor(COL_DIVIDER);
//        line->setMargin(YGEdgeTop, 4);
//        timeline->addChild(line);
//    }
//    row->addChild(timeline);

    // --- 右侧：内容卡片 ---
    // 为了美观，我们不给每个 item 加 card 背景，而是让它们像列表一样自然排列
    // 但可以给文字区加一点 padding
    auto content = LFLinear::createVertical();
    content->setFlexGrow(1.0f);
    content->setFlexShrink(1.0f);

    auto compText = createLabel(company, 14, COL_TEXT_MAIN, true);
    compText->setTextColor(0xFF000000);
    compText->setTextHAlign(LFTextHAlign::Left);
    content->addChild(compText);

    auto roleRow = LFLinear::createHorizontal();
    roleRow->setDistribution(LFDistribution::SpaceBetween);
    roleRow->matchParentWidth();
    roleRow->setMargin(YGEdgeTop, 12);

    auto roleText = createLabel(role, 12, COL_TEXT_SUB);
    roleText->setTextHAlign(LFTextHAlign::Left);
    roleText->setTextColor(0xFF444444);
    auto timeText = createLabel(time, 12, 0xFF999999);
    timeText->setTextHAlign(LFTextHAlign::Right);
    timeText->setTextColor(0xFF444444);
    roleRow->addChild(roleText);
    roleRow->addChild(timeText);
    content->addChild(roleRow);

//    auto descText = createLabel(desc, 14, 0xFF555555);
//    descText->setLineHeight(1.5f);
//    descText->setTextHAlign(LFTextHAlign::Left);
//    descText->setMargin(YGEdgeTop, 8);
//    descText->setMargin(YGEdgeTop, 16);
//    content->addChild(descText);

    row->setPadding(YGEdgeVertical, 16);
    row->setMargin(YGEdgeBottom, 16);

    row->addChild(content);
    return row;
}

std::shared_ptr<LFNode> MyProfilePage::createEducationCard(
        const std::string& universityText,
        const std::string& timeText,
        const std::string& deptText,
        const std::string& degreeText,
        const std::string& logoSrc,
        bool isLast
) {
    auto card = LFLinear::createHorizontal();
    card->matchParentWidth();
    card->setHeight(80);
//    card->setBackgroundColor(COL_CARD_BG);
//    card->setBorderRadius(CARD_RADIUS);
//    card->setShadow(0, 2, 8, 0, 0x0D000000);
//    card->setPadding(YGEdgeAll, 16);
//    card->setShadow(0, 4, 12, 0, COL_SHADOW);
    card->setDistribution(LFDistribution::Pack);

//    // --- 左侧：时间轴装饰 ---
//    auto timeline = LFLinear::createVertical();
//    timeline->setWidth(24);
//    timeline->setAlignItems(YGAlignCenter);
//    timeline->setMargin(YGEdgeRight, 0);
//
//    // 圆点
//    auto dot = LFBox::create();
//    dot->setWidth(10);
//    dot->setHeight(10);
//    dot->setBorderRadius(5);
//    dot->setBackgroundColor(COL_PRIMARY);
//    // 外圈光晕
//    dot->setBorder(2, 0xFFD1E1FA); // 淡蓝光晕
//    timeline->addChild(dot);
//
//    // 竖线
//    if (!isLast) {
//        auto line = LFBox::create();
//        line->setWidth(2);
//        line->matchParentHeight();
////        line->setFlexGrow(1.0f);
//        line->setBackgroundColor(COL_DIVIDER);
//        line->setMargin(YGEdgeTop, 4);
//        timeline->addChild(line);
//    }
//    card->addChild(timeline);

    auto row = LFLinear::createHorizontal();
//    row->setAlignItems(YGAlignCenter);
    row->setGravity(LFAlignment::Start, LFAlignment::Center);
    row->setFlexGrow(1.0f);
    row->setFlexShrink(1.0f);
    row->setBackgroundColor(COL_CARD_BG);
    row->setBorderRadius(CARD_RADIUS);
    row->setPadding(YGEdgeAll, 16);
    row->setShadow(0, 4, 12, 0, COL_SHADOW);

    // 学校 Logo (模拟)
    auto logo = std::make_shared<LFImage>();
    logo->setWidth(50);
    logo->setHeight(50);
    logo->setBorderRadius(25);
    logo->setSrc(logoSrc);
    logo->setFit(LFImageFit::Fill);
    row->addChild(logo);

    // 文字信息
    auto info = LFLinear::createVertical();
    info->setMargin(YGEdgeLeft, 12);
    info->setFlexGrow(1.0f);
    info->setFlexShrink(1.0f);

    auto universityRow = LFLinear::createHorizontal();
    universityRow->setDistribution(LFDistribution::SpaceBetween);
    universityRow->setFlexGrow(1.0f);
    universityRow->setFlexShrink(1.0f);

    auto university = createLabel(universityText, 16, COL_TEXT_MAIN, true);
    university->setTextHAlign(LFTextHAlign::Left);
    university->setTextVAlign(LFTextVAlign::Center);
    universityRow->addChild(university);

    // 时间
    auto time = createLabel(timeText, 14, 0xFF999999);
    time->setTextHAlign(LFTextHAlign::Right);
    time->setTextVAlign(LFTextVAlign::Center);
    universityRow->addChild(time);

    info->addChild(universityRow);

    auto degreeRow = LFLinear::createHorizontal();
    degreeRow->setMargin(YGEdgeTop, 12);
    degreeRow->setDistribution(LFDistribution::SpaceBetween);
    degreeRow->setFlexGrow(1.0f);
    degreeRow->setFlexShrink(1.0f);

    auto dept = createLabel(deptText, 14, COL_TEXT_SUB);
    dept->setTextHAlign(LFTextHAlign::Left);
    dept->setTextVAlign(LFTextVAlign::Center);
    degreeRow->addChild(dept);

    auto degree = createLabel(degreeText, 14, COL_TEXT_SUB);
    degree->setTextHAlign(LFTextHAlign::Right);
    degree->setTextVAlign(LFTextVAlign::Center);
    degreeRow->addChild(degree);

    info->addChild(degreeRow);

    row->addChild(info);

    card->addChild(row);
    return card;
}

std::shared_ptr<LFNode> MyProfilePage::createProjectCard(
        const std::string& title,
        const std::string& tags,
        const std::string& summary,
        const std::string& fullDetail)
{
    // 这里使用 LFButton 作为容器，天然支持点击反馈！
    // 我们的 LFButton 继承自 LFBox，所以布局能力一样
//    auto card = LFButton::create();
    auto card = LFLinear::createVertical();
    card->matchParentWidth();
    card->wrapContentHeight();
    card->setBackgroundColor(COL_CARD_BG);
//    card->setBackgroundColor(LFButtonState::Normal, COL_CARD_BG);
//    card->setBackgroundColor(LFButtonState::Pressed, 0xFFF2F5F8); // 按下变灰
    card->setBorderRadius(CARD_RADIUS);
    card->setShadow(0, 3, 10, 0, 0x14000000); // 稍微深一点的阴影体现层级
    card->setPadding(YGEdgeAll, 16);
    card->setMargin(YGEdgeBottom, 15);

    // 点击跳转逻辑
    std::weak_ptr<LFNode> weakSelf = shared_from_this();
//    card->setOnClick([weakSelf, title, fullDetail](LFButton* btn) {
//        if (auto self = std::dynamic_pointer_cast<MyProfilePage>(weakSelf.lock())) {
//            if (auto nav = self->getNavigator()) {
//                // 打开详情页
//                // nav->push(ProjectDetailPage::create(title, fullDetail));
//            }
//        }
//    });

    // 内部布局
    auto layout = LFLinear::createVertical();
    layout->matchParentWidth();

    // 1. 标题行
    auto topRow = LFLinear::createHorizontal();
    topRow->setDistribution(LFDistribution::SpaceBetween);
    topRow->matchParentWidth();

    auto t = createLabel(title, 17, COL_TEXT_MAIN, true);
    t->setTextHAlign(LFTextHAlign::Left);
    topRow->addChild(t);

//    auto arrow = createLabel(">", 16, 0xFFCCCCCC);
//    topRow->addChild(arrow);
    layout->addChild(topRow);

    // 2. 技术栈
    auto tagTxt = createLabel(tags, 12, COL_PRIMARY);
    tagTxt->setTextHAlign(LFTextHAlign::Left);
    tagTxt->setMargin(YGEdgeTop, 12);
    layout->addChild(tagTxt);

    // 3. 简介
    auto sumTxt = createLabel(summary, 14, 0xFF555555);
    sumTxt->setTextHAlign(LFTextHAlign::Left);
    sumTxt->setLineHeight(1.4f);
    sumTxt->setMargin(YGEdgeTop, 10);
    layout->addChild(sumTxt);

    if (!fullDetail.empty()) {
        auto note = createLabel(fullDetail, 13, 0xFF555555);
        note->setTextHAlign(LFTextHAlign::Left);
        note->setTextColor(0xFFFF0000);
        note->setMargin(YGEdgeTop, 16);
        layout->addChild(note);
    }

//    card->addChild(layout, LFBoxAlign::TopLeft);
    card->addChild(layout);
    return card;
}

std::shared_ptr<LFNode> buildICP() {
    auto row = LFLinear::createHorizontal();
    row->matchParentWidth();
    row->wrapContentHeight();
    row->setMargin(YGEdgeTop, 20);
    row->setGravity(LFAlignment::Center, LFAlignment::Center);
    auto icp = createLabel("", 14, 0xFF999999);
    icp->setTextHAlign(LFTextHAlign::Center);
    icp->setTextVAlign(LFTextVAlign::Center);
    row->addChild(icp);
    return row;
}
