//
// Created by Chen Tong on 2026/2/2.
//

#ifndef PROFILEPAGE_H
#define PROFILEPAGE_H

#include "LFJSONParser.h"
#include "component/LFPage.h"
#include "component/LFScrollView.h"
#include "component/LFLinear.h"
#include "component/LFBox.h"
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

    // 个人信息
    std::shared_ptr<LFNode> createHeaderSection();

    // 统计栏
    std::shared_ptr<LFNode> createStatsBar();

    // 章节标题
    std::shared_ptr<LFNode> createSectionTitle(const std::string& title);

    // 教育背景
    std::shared_ptr<LFNode> createEducationCard(
            const std::string& university,
            const std::string& time,
            const std::string& dept,
            const std::string& degree,
            const std::string& logo
    );

    // 工作经历条目
    std::shared_ptr<LFNode> createWorkExperienceItem(
            const std::string& company,
            const std::string& role,
            const std::string& time,
            const std::string& logoSrc
    );

    // 项目经历
    std::shared_ptr<LFNode> createProjectCard(
            const std::string& title,
            const std::string& tags,
            const std::string& info,
            const std::string& note
    );

    // 技能标签
    std::shared_ptr<LFNode> createSkillCloud();

    // 样式常量
    const float CARD_RADIUS = 12.0f;
    const float PAGE_PADDING = 20.0f;

    std::shared_ptr<LFJSONObject> data;
};

#endif // PROFILEPAGE_H
