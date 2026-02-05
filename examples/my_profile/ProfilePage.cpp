//
// Created by Chen Tong on 2026/2/2.
//

#include "ProfilePage.h"

std::shared_ptr<LFNode> buildICP(std::string icpText);

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

// 颜色配置
static const uint32_t COL_BG          = 0xFFF5F7FA; // 页面背景
static const uint32_t COL_CARD_BG     = 0xFFFFFFFF; // 卡片背景
static const uint32_t COL_PRIMARY     = 0xFF0052D9; // 科技蓝
static const uint32_t COL_TEXT_MAIN   = 0xFF1D1D1F; // 主要文字
static const uint32_t COL_TEXT_SUB    = 0xFF6E6E73; // 次要文字
static const uint32_t COL_DIVIDER     = 0xFFE5E5E5; // 分割线
static const uint32_t COL_SHADOW      = 0x1A000000; // 阴影颜色 (10%透明度)

// 字体大小
static const float FS_TITLE  = 22.0f;
static const float FS_HEAD   = 18.0f;
static const float FS_BODY   = 15.0f;
static const float FS_SMALL  = 13.0f;

std::shared_ptr<ProfilePage> ProfilePage::create() {
    auto page = std::make_shared<ProfilePage>();
    LFResourceProvider::getInstance().fetchAsset("profile.json", [page](std::shared_ptr<LFData> data) {
        std::string jsonCode = reinterpret_cast<const char*>(data->data);
        page->data = LFJSONParser::parse(jsonCode);
        if (page->data) {
            page->initUI();
        }
    });
    return page;
}

ProfilePage::ProfilePage() {
    // 构造函数
}

void ProfilePage::onEnter() {
    LFPage::onEnter();
    // 可以在这里做埋点统计
}

void ProfilePage::initUI() {
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

    // B. 核心指标栏
    content->addChild(createStatsBar());

    // C. 专业技能
    content->addChild(createSectionTitle(data->at("skill").at("title").asString()));
    content->addChild(createSkillCloud());

    // D. 工作经历
    content->addChild(createSectionTitle(data->at("work").at("title").asString()));
    // 这种容器用于绘制左侧的时间轴线
    auto workContainer = LFLinear::createVertical();
    workContainer->matchParentWidth();
    auto workList = data->at("work").at("list").asArray();
    for (auto item : workList) {
        workContainer->addChild(createWorkExperienceItem(
        item.at("company").asString(),
        item.at("role").asString(),
        item.at("time").asString(),
        item.at("logoSrc").asString()
        ));
    }
    content->addChild(workContainer);

    // E. 教育背景
    content->addChild(createSectionTitle(data->at("edu").at("title").asString()));
    auto eduContainer = LFLinear::createVertical();
    eduContainer->matchParentWidth();
    eduContainer->setSpacing(16);
    auto eduList = data->at("edu").at("list").asArray();
    for (int i = 0; i < eduList.size(); i++) {
        auto item = eduList[i];
        eduContainer->addChild(createEducationCard(
            item.at("school").asString(),
            item.at("time").asString(),
            item.at("department").asString(),
            item.at("degree").asString(),
            item.at("logoSrc").asString()
        ));
    }
    eduContainer->setMargin(YGEdgeBottom, 16);
    content->addChild(eduContainer);

    // F. 项目经历
    content->addChild(createSectionTitle(data->at("project").at("title").asString()));
    auto projectList = data->at("project").at("list").asArray();
    for (auto item : projectList) {
        auto respList = item.at("resp").asArray();
        auto tl = std::vector<std::string>();
        if (!respList.empty()) {
            for (auto t : respList) {
                tl.push_back(t.asString());
            }
        }
        content->addChild(createProjectCard(
            item.at("title").asString(),
            data->at("project").at("resp-title").asString(),
            data->at("project").at("url-title").asString(),
            item.at("tags").asString(),
            item.at("info").asString(),
            item.at("note").asString(),
            tl,
            item.at("url").asString()
        ));
    }

    // ICP 备案信息
    auto icpText = data->at("icp");
    if (!icpText.asString().empty()) {
        content->addChild(buildICP(icpText.asString()));
    }

    scrollView->addChild(content);
}

// ==========================================
// 组件实现细节
// ==========================================

std::shared_ptr<LFNode> ProfilePage::createHeaderSection() {
    auto card = LFLinear::createVertical();
    card->matchParentWidth();
    card->setHeight(200);
    card->setMargin(YGEdgeBottom, 16);
    card->setGravity(LFAlignment::Center, LFAlignment::Center);

    auto column = LFLinear::createVertical();
    column->setSpacing(10);
    column->matchParentWidth();
    column->matchParentHeight();
    column->setAlignItems(YGAlignCenter); // 垂直居中
    column->setGravity(LFAlignment::Center, LFAlignment::Center);

    // 头像
    auto avatar = std::make_shared<LFImage>();
    avatar->setSrc(data->at("avatar").asString());
    avatar->setWidth(80);
    avatar->setHeight(80);
    avatar->setBorderRadius(40); // 圆形
    avatar->setBackgroundColor(0xFFE0E0E0); // 占位色
    avatar->setBorder(2.0f, 0xFFFFFFFF); // 白圈描边，增加精致感
    avatar->setShadow(0, 2, 8, 0, 0x33000000);
    column->addChild(avatar);

    auto name = createLabel(data->at("name").asString(), 26, COL_TEXT_MAIN, true);
    name->setTextHAlign(LFTextHAlign::Center);
    name->setMargin(YGEdgeTop, 20);
    column->addChild(name);

    auto role = createLabel(data->at("role").asString(), 15, COL_TEXT_SUB);
    role->setTextHAlign(LFTextHAlign::Center);
    role->setMargin(YGEdgeTop, 5);
    column->addChild(role);

    auto contact = std::make_shared<LFText>();
    contact->wrapContentHeight();
    contact->setTextHAlign(LFTextHAlign::Center);
    contact->setTextVAlign(LFTextVAlign::Center);
    contact->setText(data->at("email").asString());
    contact->setTextColor(COL_PRIMARY);
    contact->setFontSize(11);

    column->addChild(contact);
    card->addChild(column);

    return card;
}

std::shared_ptr<LFNode> ProfilePage::createStatsBar() {
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

    auto statList = data->at("stats").asArray();
    for (auto stat : statList) {
        container->addChild(makeStat(stat.at("value").asString(), stat.at("label").asString()));
    }

    return container;
}

std::shared_ptr<LFNode> ProfilePage::createSectionTitle(const std::string& title) {
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

std::shared_ptr<LFNode> ProfilePage::createSkillCloud() {
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

std::shared_ptr<LFNode> ProfilePage::createWorkExperienceItem(
        const std::string& company,
        const std::string& role,
        const std::string& time,
        const std::string& logoSrc
)
{

    auto row = LFLinear::createHorizontal();
    row->matchParentWidth();
    row->setHeight(80);
    row->setSpacing(10);
    row->setGravity(LFAlignment::Start, LFAlignment::Center);
    row->setPadding(YGEdgeHorizontal, 16);
    row->setBackgroundColor(COL_CARD_BG);
    row->setBorderRadius(CARD_RADIUS);
    row->setShadow(0, 4, 12, 0, COL_SHADOW);

    auto logo = std::make_shared<LFImage>();
    logo->setFit(LFImageFit::Fill);
    logo->setSrc(logoSrc);
    logo->setWidth(50);
    logo->setHeight(50);
    logo->setBorderRadius(CARD_RADIUS);
    logo->setBorder(1, 0xFFD1E1FA);
    row->addChild(logo);

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

    row->setPadding(YGEdgeVertical, 16);
    row->setMargin(YGEdgeBottom, 16);

    row->addChild(content);
    return row;
}

std::shared_ptr<LFNode> ProfilePage::createEducationCard(
        const std::string& universityText,
        const std::string& timeText,
        const std::string& deptText,
        const std::string& degreeText,
        const std::string& logoSrc
) {
    auto card = LFLinear::createHorizontal();
    card->matchParentWidth();
    card->setHeight(80);
    card->setDistribution(LFDistribution::Pack);

    auto row = LFLinear::createHorizontal();
    row->setGravity(LFAlignment::Start, LFAlignment::Center);
    row->setFlexGrow(1.0f);
    row->setFlexShrink(1.0f);
    row->setBackgroundColor(COL_CARD_BG);
    row->setBorderRadius(CARD_RADIUS);
    row->setPadding(YGEdgeAll, 16);
    row->setShadow(0, 4, 12, 0, COL_SHADOW);

    auto logo = std::make_shared<LFImage>();
    logo->setWidth(50);
    logo->setHeight(50);
    logo->setBorderRadius(25);
    logo->setSrc(logoSrc);
    logo->setFit(LFImageFit::Fill);
    row->addChild(logo);

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

std::shared_ptr<LFNode> ProfilePage::createProjectCard(
        const std::string& title,
        const std::string& respLabel,
        const std::string& urlLabel,
        const std::string& tags,
        const std::string& info,
        const std::string& noteText,
        const std::vector<std::string> resp,
        std::string url)
{

    auto card = LFLinear::createVertical();
    card->matchParentWidth();
    card->wrapContentHeight();
    card->setBackgroundColor(COL_CARD_BG);
    card->setBorderRadius(CARD_RADIUS);
    card->setShadow(0, 3, 10, 0, 0x14000000); // 稍微深一点的阴影体现层级
    card->setPadding(YGEdgeAll, 16);
    card->setMargin(YGEdgeBottom, 15);

    // 内部布局
    auto layout = LFLinear::createVertical();
    layout->matchParentWidth();

    auto topRow = LFLinear::createHorizontal();
    topRow->setGravity(LFAlignment::Center, LFAlignment::Center);
    topRow->setDistribution(LFDistribution::SpaceBetween);
    topRow->matchParentWidth();

    auto t = createLabel(title, 17, COL_TEXT_MAIN, true);
    t->setTextHAlign(LFTextHAlign::Left);
    topRow->addChild(t);

    auto add = std::make_shared<LFImage>();
    add->setSrc("arrow-right.png");
    add->setWidth(16);
    add->setHeight(16);
    add->setFit(LFImageFit::Fill);
    add->setVisible(false);
    topRow->addChild(add);

    layout->addChild(topRow);

    auto tagTxt = createLabel(tags, 12, COL_PRIMARY);
    tagTxt->setTextHAlign(LFTextHAlign::Left);
    tagTxt->setMargin(YGEdgeTop, 12);
    layout->addChild(tagTxt);

    auto sumTxt = createLabel(info, 14, 0xFF555555);
    sumTxt->setTextHAlign(LFTextHAlign::Left);
    sumTxt->setLineHeight(1.4f);
    sumTxt->setMargin(YGEdgeTop, 10);
    layout->addChild(sumTxt);

    if (!noteText.empty()) {
        auto note = createLabel(noteText, 13, 0xFF555555);
        note->setTextHAlign(LFTextHAlign::Left);
        note->setTextColor(0xFFFF0000);
        note->setMargin(YGEdgeTop, 16);
        layout->addChild(note);
    }

    card->addChild(layout);

    // 主要职责
    if (!resp.empty() || !url.empty()) {
        add->setVisible(true);
        // 展开容器
        auto respBox = LFLinear::createVertical();
        respBox->matchParentWidth();
        respBox->wrapContentHeight();
        respBox->setGravity(LFAlignment::Start, LFAlignment::Start);
        respBox->setMasksToBounds(true);         // 关键：裁剪内容，实现高度动画
        respBox->setHeight(0);                   // 初始高度为 0 (折叠状态)
        respBox->setOpacity(0);                  // 初始透明度 0

        auto divide = std::make_shared<LFNode>();
        divide->matchParentWidth();
        divide->setHeight(16);
        divide->setBackgroundColor(0x00000000);
        respBox->addChild(divide);

        // 内容填充
        auto respContent = LFLinear::createVertical();
        respContent->setGravity(LFAlignment::Start, LFAlignment::Start);
        respContent->setBorderRadius(CARD_RADIUS);
        respContent->setBackgroundColor(0xFFF7F8FA); // 浅灰背景，与白卡区分
        respContent->matchParentWidth();
        respContent->wrapContentHeight();
        respContent->setPadding(YGEdgeAll, 16); // 内部留白

        if (!resp.empty()) {
            // 职责标题
            auto rTitle = createLabel(respLabel, 13, COL_TEXT_MAIN, true);
            rTitle->setTextHAlign(LFTextHAlign::Left);
            rTitle->setMargin(YGEdgeBottom, 10);
            respContent->addChild(rTitle);

            // 职责列表
            for (const auto& itemStr : resp) {
                auto row = LFLinear::createHorizontal();
                row->setMargin(YGEdgeBottom, 6);
                row->setGravity(LFAlignment::Start, LFAlignment::Start);

                // 圆点
                auto dot = createLabel("•", 14, COL_PRIMARY);
                dot->setMargin(YGEdgeRight, 8);

                // 文字
                auto text = createLabel(itemStr, 13, 0xFF666666);
                text->setTextHAlign(LFTextHAlign::Left);
                text->setTextVAlign(LFTextVAlign::Center);
                text->setLineHeight(1.4f);
                text->setFlexGrow(1.0f);
                text->setFlexShrink(1); // 允许换行

                row->addChild(dot);
                row->addChild(text);
                respContent->addChild(row);
            }
        }

        if (!url.empty()) {
            auto urlRow = LFLinear::createHorizontal();
            urlRow->matchParentWidth();
            urlRow->wrapContentHeight();
            // 如果上面有列表，给个顶部间距；如果只是链接，就不需要间距
            float topMargin = resp.empty() ? 0.0f : 10.0f;
            urlRow->setMargin(YGEdgeTop, topMargin);
            urlRow->setAlignItems(YGAlignFlexStart);

            // 标签
            auto label = createLabel(urlLabel, 13, COL_TEXT_MAIN);
            label->setTextHAlign(LFTextHAlign::Left);

            // 链接
            auto link = createLabel(url, 13, COL_PRIMARY); // 品牌蓝
            link->setTextHAlign(LFTextHAlign::Left);
            link->matchParentWidth();
            link->setFlexGrow(1.0f);
            link->setFlexShrink(1); // URL太长允许换行

            urlRow->addChild(label);
            urlRow->addChild(link);
            respContent->addChild(urlRow);
        }

        respBox->addChild(respContent);
        card->addChild(respBox);

        // 展开/收起
        // 使用 shared_ptr 捕获状态，使得状态能跟随卡片生命周期
        auto isExpanded = std::make_shared<bool>(false);
        std::weak_ptr<LFLinear> weakResp = respBox;
        std::weak_ptr<LFImage> weakAdd = add;

        card->setOnTap([card, weakAdd, weakResp, isExpanded](const LFPoint& p) {
            auto box = weakResp.lock();
            if (!box) return;

            // 切换状态
            bool expanding = !(*isExpanded);
            *isExpanded = expanding;

            // 确定动画起止点
            float startH = box->getLayoutHeight();
            float endH = 0.0f;

            if (*isExpanded) {
                box->wrapContentHeight();
                float availableWidth = card->getLayoutWidth();
                // 减去左右 Padding 的大小
                box->calculateLayout(availableWidth - 32, NAN);
                box->setHeight(startH);
                endH = box->getLayoutHeight();
            }

            // 创建高度动画
            auto anim = LFValueAnimator<float>::of(startH, endH);
            anim->setDuration(0.3f); // 300ms
            anim->setEasing(LFEasingType::QuadOut); // 舒缓的减速曲线

            float startR, endR;
            if (*isExpanded) {
                startR = 0;
                endR = 90;
            } else {
                startR = 90;
                endR = 0;
            }
            auto addAnim = LFValueAnimator<float>::of(startR, endR);
            addAnim->setDuration(0.3f); // 300ms
            addAnim->setEasing(LFEasingType::QuadOut); // 舒缓的减速曲线

            // 更新回调
            anim->addUpdateListener([weakResp, expanding](const float& h) {
                if (auto b = weakResp.lock()) {
                    b->setHeight(h);
                    // 顺便做一个透明度渐变，体验更丝滑
                    // 展开时：透明度随高度增加；收起时：随高度减小
                    // 简单映射：高度 > 10px 开始显示
                    float alpha = std::min(1.0f, h / 50.0f);
                    b->setOpacity(alpha);
                }
            });

            addAnim->addUpdateListener([weakAdd, expanding](const float& r) {
                if (auto i = weakAdd.lock()) {
                    i->setRotate(r);
                }
            });

            anim->setOnEnd([weakResp, expanding]() {
                if (expanding) {
                    if (auto b = weakResp.lock()) {
                        b->wrapContentHeight(); // 设回 Auto
                    }
                }
            });

            anim->start();
            addAnim->start();
            // 注册到全局管理器以保持动画存活
            LFGlobalAnimationManager::getInstance().addAnimator(anim);
            LFGlobalAnimationManager::getInstance().addAnimator(addAnim);
        });
    }

    return card;
}

std::shared_ptr<LFNode> buildICP(std::string icpText) {
    auto row = LFLinear::createHorizontal();
    row->matchParentWidth();
    row->wrapContentHeight();
    row->setMargin(YGEdgeTop, 20);
    row->setGravity(LFAlignment::Center, LFAlignment::Center);
    auto icp = createLabel(icpText, 14, 0xFF999999);
    icp->setTextHAlign(LFTextHAlign::Center);
    icp->setTextVAlign(LFTextVAlign::Center);
    row->addChild(icp);
    return row;
}
