//
// Created by Chen Tong on 2026/2/2.
//

#ifndef PROFILEPAGE_H
#define PROFILEPAGE_H

#include "LFPage.h"
#include "LFScrollView.h"
#include "LFLinear.h"
#include "LFBox.h"
#include <string>
#include <vector>

class MyProfilePage : public LFPage {
public:
    // 工厂方法
    static std::shared_ptr<MyProfilePage> create();

    MyProfilePage();
    virtual ~MyProfilePage() = default;

    // 生命周期
    void onEnter() override;

private:
    void initUI();

    // --- UI 构建辅助方法 (Private Helpers) ---
    // 为了保持 initUI 整洁，将各板块的构建逻辑拆分

    // 1. 头部个人信息卡片
    std::shared_ptr<LFNode> createHeaderSection();

    // 2. 统计栏 (年龄/经验等)
    std::shared_ptr<LFNode> createStatsBar();

    // 3. 章节标题
    std::shared_ptr<LFNode> createSectionTitle(const std::string& title);

    // 4. 教育背景卡片
    std::shared_ptr<LFNode> createEducationCard(
            const std::string& university,
            const std::string& time,
            const std::string& dept,
            const std::string& degree,
            const std::string& logo,
            bool isLast = true
    );

    // 5. 工作经历条目 (时间轴样式)
    std::shared_ptr<LFNode> createWorkExperienceItem();

    // 6. 项目经历卡片 (可点击)
    std::shared_ptr<LFNode> createProjectCard(
            const std::string& title,
            const std::string& tags,
            const std::string& summary,
            const std::string& fullDetail
    );

    // 7. 通用技能标签云
    std::shared_ptr<LFNode> createSkillCloud();

    // --- 样式常量 ---
    const float CARD_RADIUS = 12.0f;
    const float PAGE_PADDING = 20.0f;
};

#endif // PROFILEPAGE_H
