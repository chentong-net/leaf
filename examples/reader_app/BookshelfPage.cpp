//
// Created by Chen Tong on 2026/2/2.
//

#include "BookshelfPage.h"

#include "AppUtils.h"
#include "BookContentLoader.h"
#include "BookImportService.h"
#include "BookRepository.h"
#include "ReaderPage.h"
#include "ReadingProgressStore.h"

#include "LFPathProvider.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace {

class DashedBorderBox : public LFBox {
public:
    using Ptr = std::shared_ptr<DashedBorderBox>;

    static Ptr create() {
        return std::make_shared<DashedBorderBox>();
    }

    void setDashStyle(float dash, float gap, float width, uint32_t color) {
        m_dash = dash;
        m_gap = gap;
        m_width = width;
        m_color = color;
    }

protected:
    void onDrawOverlay(NVGcontext* vg) override {
        const float w = getLayoutWidth();
        const float h = getLayoutHeight();
        if (w <= 2.0f || h <= 2.0f) return;

        auto drawDashed = [this, vg](float x1, float y1, float x2, float y2) {
            const float dx = x2 - x1;
            const float dy = y2 - y1;
            const float len = std::sqrt(dx * dx + dy * dy);
            if (len <= 0.0f) return;

            const float ux = dx / len;
            const float uy = dy / len;
            for (float pos = 0.0f; pos < len; pos += (m_dash + m_gap)) {
                const float endPos = std::min(pos + m_dash, len);
                nvgMoveTo(vg, x1 + ux * pos, y1 + uy * pos);
                nvgLineTo(vg, x1 + ux * endPos, y1 + uy * endPos);
            }
        };

        nvgBeginPath(vg);
        const float half = m_width * 0.5f;
        drawDashed(half, half, w - half, half);
        drawDashed(w - half, half, w - half, h - half);
        drawDashed(w - half, h - half, half, h - half);
        drawDashed(half, h - half, half, half);
        nvgStrokeColor(vg, LFNode::colorToNVG(m_color));
        nvgStrokeWidth(vg, m_width);
        nvgLineCap(vg, NVG_BUTT);
        nvgStroke(vg);
    }

private:
    float m_dash = 8.0f;
    float m_gap = 6.0f;
    float m_width = 1.5f;
    uint32_t m_color = 0xFFB7B7B7;
};

uint32_t coverColorByIndex(int index) {
    static constexpr uint32_t kColors[] = {
        0xFFFFE2E2, 0xFFE3F2FD, 0xFFE8F5E9, 0xFFFFF3E0, 0xFFEDE7F6
    };
    return kColors[index % (sizeof(kColors) / sizeof(kColors[0]))];
}

std::string makeProgressStorePath(const std::string& appSupportPath) {
    if (appSupportPath.empty()) return "";
    if (appSupportPath.back() == '/' || appSupportPath.back() == '\\') {
        return appSupportPath + "reader_app/reading_progress.json";
    }
    return appSupportPath + "/reader_app/reading_progress.json";
}

double nowSeconds() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count() / 1000.0;
}

constexpr float kBookshelfItemWidth = 72.0f;
constexpr float kBookshelfCoverAspectRatio = 0.75f; // 3:4
constexpr float kBookshelfGridSpacing = 15.0f;
constexpr float kBookshelfGridPadding = 15.0f;

} // namespace

std::shared_ptr<BookshelfPage> BookshelfPage::create(std::weak_ptr<LFNavigator> nav) {
    auto page = std::make_shared<BookshelfPage>();
    page->m_navigator = nav;
    page->setBackgroundColor(0xFFF0F0F0);
    page->initUI();
    page->setupStorage();
    return page;
}

void BookshelfPage::initUI() {
    auto navbar = LFLinear::createHorizontal();
    navbar->matchParentWidth();
    navbar->setHeight(56);
    navbar->setBackgroundColor(0xFFFFFFFF);
    navbar->setPadding(YGEdgeHorizontal, 16);
    navbar->setGravity(LFAlignment::Start, LFAlignment::Center);
    navbar->setBorder(1.0f, 0xFFE0E0E0);
    YGNodeStyleSetBorder(navbar->getYGNode(), YGEdgeBottom, 0);

    auto title = AppUtils::createLabel("我的书架", 18, 0xFF000000);
    title->setTextHAlign(LFTextHAlign::Left);
    navbar->addChild(title);
    addChild(navbar);

    m_statusText = AppUtils::createLabel("准备中...", 12, 0xFF999999);
    m_statusText->matchParentWidth();
    m_statusText->setHeight(22);
    m_statusText->setPadding(YGEdgeHorizontal, 16);
    m_statusText->setTextHAlign(LFTextHAlign::Left);
    addChild(m_statusText);

    auto scrollView = LFScrollView::createVertical();
    scrollView->setFlexGrow(1.0f);
    scrollView->setFlexBasis(0);
    scrollView->matchParentWidth();
    scrollView->setBounces(false);
    scrollView->setScrollBarEnabled(false);

    m_grid = LFGrid::create(1, kBookshelfGridSpacing);
    m_grid->matchParentWidth();
    m_grid->setPadding(YGEdgeAll, kBookshelfGridPadding);
    scrollView->addChild(m_grid);
    addChild(scrollView);
}

void BookshelfPage::setupStorage() {
    std::weak_ptr<BookshelfPage> weakSelf = std::static_pointer_cast<BookshelfPage>(shared_from_this());
    LFPathProvider::getApplicationSupportPath([weakSelf](const LFPathProviderResult& result) {
        auto self = weakSelf.lock();
        if (!self) return;

        if (!result.ok || result.path.empty()) {
            self->setStatusText("初始化失败：无法获取应用目录", 0xFFFF4D4F);
            self->refreshGrid();
            return;
        }

        auto& repository = BookRepository::getInstance();
        if (!repository.initialize(result.path)) {
            self->setStatusText("初始化失败：无法创建书架目录", 0xFFFF4D4F);
            self->refreshGrid();
            return;
        }

        self->m_booksDirectory = repository.getBooksDirectory();
        FileReadingProgressStore::getInstance().initialize(makeProgressStorePath(result.path));
        self->m_books = repository.listBooks();
        self->refreshGrid();

        if (self->m_books.empty()) {
            self->setStatusText("点击“+”导入书籍", 0xFF999999);
        } else {
            self->setStatusText("已加载 " + std::to_string(self->m_books.size()) + " 本书", 0xFF52C41A);
        }
    });
}

void BookshelfPage::refreshGrid() {
    if (!m_grid) return;

    for (const auto& item : m_gridItems) {
        m_grid->removeChild(item);
    }
    m_gridItems.clear();

    addImportEntry();
    for (int i = 0; i < static_cast<int>(m_books.size()); ++i) {
        addBookEntry(m_books[i], i);
    }

    updateGridColumnsForLayoutWidth(m_grid->getLayoutWidth());
}

void BookshelfPage::addImportEntry() {
    auto itemLayout = LFLinear::createVertical();
    itemLayout->setWidth(kBookshelfItemWidth);
    itemLayout->wrapContentHeight();
    itemLayout->setGravity(LFAlignment::Center, LFAlignment::Center);
    itemLayout->setSpacing(12.0f);
    itemLayout->setAlignSelf(YGAlignCenter);

    auto cover = DashedBorderBox::create();
    cover->matchParentWidth();
    cover->setAspectRatio(kBookshelfCoverAspectRatio);
    cover->setRadius(8.0f);
    cover->setDashStyle(8.0f, 5.0f, 1.5f, 0xFFB7B7B7);
    cover->setBackgroundColor(0xFFFDFDFD);

    auto addIcon = std::make_shared<LFImage>();
    addIcon->setSrc("add.png");
    addIcon->setWidth(28);
    addIcon->setHeight(28);
    addIcon->setFit(LFImageFit::Fill);
    cover->addChild(addIcon, LFBoxAlign::Center);

    auto title = AppUtils::createLabel("导入书籍", 12, 0xFF666666);
    title->setTextHAlign(LFTextHAlign::Center);

    itemLayout->addChild(cover);
    itemLayout->addChild(title);

    std::weak_ptr<BookshelfPage> weakSelf = std::static_pointer_cast<BookshelfPage>(shared_from_this());
    itemLayout->setOnTap([weakSelf](const LFPoint&) {
        if (auto self = weakSelf.lock()) {
            self->startImportBook();
        }
    });

    m_grid->addChild(itemLayout);
    m_gridItems.push_back(itemLayout);
}

void BookshelfPage::addBookEntry(const BookRecord& book, int index) {
    auto itemLayout = LFLinear::createVertical();
    itemLayout->setWidth(kBookshelfItemWidth);
    itemLayout->wrapContentHeight();
    itemLayout->setGravity(LFAlignment::Center, LFAlignment::Center);
    itemLayout->setSpacing(12.0f);
    itemLayout->setAlignSelf(YGAlignCenter);

    auto cover = LFBox::create();
    cover->matchParentWidth();
    cover->setAspectRatio(kBookshelfCoverAspectRatio);
    cover->setBackgroundColor(coverColorByIndex(index));
    cover->setRadius(8.0f);
    cover->setShadow(0, 2, 6, 0, 0x22000000);

    auto coverText = AppUtils::createLabel(book.title, 14, 0xFF444444);
    coverText->matchParentWidth();
    coverText->setTextHAlign(LFTextHAlign::Center);
    coverText->setTextVAlign(LFTextVAlign::Center);
    coverText->setPadding(YGEdgeHorizontal, 8.0f);
    coverText->setLineHeight(1.2f);
    cover->addChild(coverText, LFBoxAlign::Center);

    auto nameText = AppUtils::createLabel(book.title, 12, 0xFF333333);
    nameText->matchParentWidth();
    nameText->setTextHAlign(LFTextHAlign::Center);

    itemLayout->addChild(cover);
    itemLayout->addChild(nameText);

    std::weak_ptr<BookshelfPage> weakSelf = std::static_pointer_cast<BookshelfPage>(shared_from_this());
    itemLayout->setOnTap([weakSelf, book](const LFPoint&) {
        if (auto self = weakSelf.lock()) {
            self->openBook(book);
        }
    });

    m_grid->addChild(itemLayout);
    m_gridItems.push_back(itemLayout);
}

void BookshelfPage::startImportBook() {
    if (m_importing) return;
    if (m_booksDirectory.empty()) {
        setStatusText("书架目录未初始化，稍后重试", 0xFFFF4D4F);
        return;
    }

    m_importing = true;
    setStatusText("正在选择文件...", 0xFF1677FF);

    std::weak_ptr<BookshelfPage> weakSelf = std::static_pointer_cast<BookshelfPage>(shared_from_this());
    BookImportService::pickAndImport(m_booksDirectory, [weakSelf](const BookImportResult& importResult) {
        auto self = weakSelf.lock();
        if (!self) return;

        self->m_importing = false;

        if (!importResult.ok) {
            if (!importResult.canceled) {
                self->setStatusText("导入失败：" + importResult.error, 0xFFFF4D4F);
            } else {
                self->setStatusText("已取消导入", 0xFF999999);
            }
            return;
        }

        auto& repository = BookRepository::getInstance();
        if (!repository.upsertBook(importResult.book)) {
            self->setStatusText("导入失败：写入书架记录失败", 0xFFFF4D4F);
            return;
        }

        self->m_books = repository.listBooks();
        self->refreshGrid();
        self->setStatusText("导入成功：" + importResult.book.title, 0xFF52C41A);
    });
}

void BookshelfPage::openBook(const BookRecord& book) {
    if (book.filePath.empty()) {
        setStatusText("打开失败：文件路径为空", 0xFFFF4D4F);
        return;
    }

    auto loadResult = BookContentLoader::loadTextFile(book.filePath);
    if (!loadResult.ok) {
        setStatusText("读取失败：" + loadResult.error, 0xFFFF4D4F);
        return;
    }

    BookRepository::getInstance().touchBook(book.id, nowSeconds());

    auto readerPage = ReaderPage::create(book.id, book.title, loadResult.content);
    if (auto nav = m_navigator.lock()) {
        nav->push(readerPage);
    } else {
        setStatusText("导航器不可用，无法打开阅读页", 0xFFFF4D4F);
    }
}

void BookshelfPage::setStatusText(const std::string& text, uint32_t color) {
    if (!m_statusText) return;
    m_statusText->setText(text);
    m_statusText->setTextColor(color);
}

void BookshelfPage::onAfterCalculateLayout() {
    LFPage::onAfterCalculateLayout();
    if (!m_grid) return;
    updateGridColumnsForLayoutWidth(m_grid->getLayoutWidth());
}

void BookshelfPage::updateGridColumnsForLayoutWidth(float gridLayoutWidth) {
    if (!m_grid || gridLayoutWidth <= 0.0f) return;

    const float contentWidth = std::max(0.0f, gridLayoutWidth - kBookshelfGridPadding * 2.0f);
    int columns = static_cast<int>(std::floor((contentWidth + kBookshelfGridSpacing)
                                              / (kBookshelfItemWidth + kBookshelfGridSpacing)));
    columns = std::max(1, columns);
    m_grid->setColumnCount(columns);
}
