//
// Created by Chen Tong on 2026/2/2.
//

#include "BookshelfPage.h"
#include "ReaderPage.h"
#include "AppUtils.h"

std::shared_ptr<BookshelfPage> BookshelfPage::create(std::weak_ptr<LFNavigator> nav) {
    auto page = std::make_shared<BookshelfPage>();
    page->m_navigator = nav;
    page->setBackgroundColor(0xFFF0F0F0); // 浅灰背景
    page->initUI();
    return page;
}

void BookshelfPage::initUI() {
    // 1. 顶部导航栏
    auto navbar = LFLinear::createHorizontal();
    navbar->matchParentWidth();
    navbar->setHeight(56);
    navbar->setBackgroundColor(0xFFFFFFFF);
    navbar->setPadding(YGEdgeHorizontal, 16);
    navbar->setGravity(LFAlignment::Start, LFAlignment::Center);
    navbar->setBorder(1.0f, 0xFFE0E0E0);
    YGNodeStyleSetBorder(navbar->getYGNode(), YGEdgeBottom, 0);

    auto title = AppUtils::createLabel("我的书架", 18, 0xFF000000);
    navbar->addChild(title);

    // 2. 滚动区域
    auto scrollView = LFScrollView::createVertical();
    scrollView->setFlexGrow(1.0f);
    scrollView->setFlexBasis(0);
    scrollView->matchParentWidth();
    scrollView->setBounces(false);
    scrollView->setScrollBarEnabled(false);

    // 3. 网格布局
    auto grid = LFGrid::create(3, 15.0f);
    grid->matchParentWidth();
    grid->setPadding(YGEdgeAll, 15.0f);

    // 4. 填充数据
    for (int i = 0; i < 15; i++) {
        auto itemLayout = LFLinear::createVertical();
        itemLayout->matchParentWidth();
        itemLayout->wrapContentHeight();
        itemLayout->setGravity(LFAlignment::Center, LFAlignment::Center);
        itemLayout->setSpacing(15.0f);

        // 封面
        auto cover = LFBox::create();
        cover->matchParentWidth();
        cover->setAspectRatio(0.75f);
        uint32_t colors[] = {0xFFFFABAB, 0xFFFFFFCC, 0xFFCCFFCC, 0xFFCCCCFF};
        cover->setBackgroundColor(colors[i % 4]);
        cover->setRadius(6.0f);
        cover->setShadow(0, 2, 5, 0, 0x33000000);

        auto coverText = AppUtils::createLabel("Book " + std::to_string(i+1), 14, 0xFF555555);
        cover->addChild(coverText, LFBoxAlign::Center);

        // 书名
        auto nameText = AppUtils::createLabel("测试书籍 " + std::to_string(i), 12, 0xFF333333);

        itemLayout->addChild(cover);
        itemLayout->addChild(nameText);

        // 点击事件
        std::weak_ptr<LFNavigator> weakNav = m_navigator;
        itemLayout->setOnTap([i, weakNav](const LFPoint& point){
            LF_LOGI("Clicked Book: %d", i);
            std::string bookTitle = "十日终焉.txt";

            LFResourceProvider::getInstance().fetchAsset(bookTitle, [bookTitle, weakNav](std::shared_ptr<LFData> data) {
                if (!data) return;

                std::string content(reinterpret_cast<const char*>(data->data), data->size);
                auto readerPage = ReaderPage::create(bookTitle, content);

                if (auto nav = weakNav.lock()) {
                    nav->push(readerPage);
                }
            });
        });

        grid->addChild(itemLayout);
    }

    scrollView->addChild(grid);

    this->addChild(navbar);
    this->addChild(scrollView);
}
