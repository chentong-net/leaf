//
// Created by Chen Tong on 2026/1/31.
//

#include "ReaderPage.h"

#include <algorithm>
#include <sstream>

const float TOP_BAR_HEIGHT = 56.0f;         // 顶部导航栏高度
const float ADAPTER_PADDING_TOP = 30.0f;    // 正文顶部内边距
const float ADAPTER_PADDING_BOTTOM = 30.0f; // 正文底部内边距
const float ADAPTER_PADDING_X = 24.0f;      // 正文左右内边距

static std::string normalizeText(const std::string& input) {
    if (input.empty()) return "";

    std::string output;
    output.reserve(input.size());
    for (char c : input) {
        if (c == '\r') continue;
        output.push_back(c);
    }
    return output;
}

/**
 * 书籍页面适配器
 * 使用 front+main 双段数据实现“从恢复点打开 + 向前补齐”
 */
class BookPageAdapter : public LFPageAdapter {
public:
    BookPageAdapter(const std::vector<SplitPage>& mainPages, float fontSize)
        : m_mainPages(mainPages), m_fontSize(fontSize) {}

    int getCount() override {
        return static_cast<int>(m_frontPages.size() + m_mainPages.size());
    }

    LFNode::Ptr createView() override {
        auto container = LFBox::create();
        container->matchParentWidth();
        container->matchParentHeight();

        auto contentText = std::make_shared<LFText>();
        contentText->setFontSize(m_fontSize);
        contentText->setLineHeight(1.6f);
        contentText->setTextColor(0xFF333333);
        contentText->setTextHAlign(LFTextHAlign::Left);
        contentText->setTextVAlign(LFTextVAlign::Top);
        contentText->matchParentWidth();
        contentText->matchParentHeight();
        contentText->setPadding(YGEdgeTop, ADAPTER_PADDING_TOP);
        contentText->setPadding(YGEdgeBottom, ADAPTER_PADDING_BOTTOM);
        contentText->setPadding(YGEdgeHorizontal, ADAPTER_PADDING_X);

        auto footerText = std::make_shared<LFText>();
        footerText->setFontSize(11.0f);
        footerText->setTextColor(0xFFAAAAAA);
        footerText->setTextHAlign(LFTextHAlign::Center);
        footerText->setTextVAlign(LFTextVAlign::Center);

        container->addChild(contentText, LFBoxAlign::MatchParent);
        container->addChild(footerText, LFBoxAlign::BottomCenter, 0, -20.0f);
        return container;
    }

    void bindView(LFNode::Ptr view, int index) override {
        auto container = std::static_pointer_cast<LFBox>(view);
        auto children = container->getChildren();
        if (children.size() < 2) return;

        auto contentText = std::static_pointer_cast<LFText>(children[0]);
        auto footerText = std::static_pointer_cast<LFText>(children[1]);

        const SplitPage* page = getPage(index);
        if (page) {
            contentText->setText(page->text);
        } else {
            contentText->setText("");
        }

        std::stringstream ss;
        ss << (index + 1) << " / " << getCount();
        footerText->setText(ss.str());
    }

    void appendMainPages(const std::vector<SplitPage>& pages) {
        if (pages.empty()) return;
        m_mainPages.insert(m_mainPages.end(), pages.begin(), pages.end());
    }

    int prependFrontPages(const std::vector<SplitPage>& pages) {
        if (pages.empty()) return 0;
        m_frontPages.insert(m_frontPages.begin(), pages.begin(), pages.end());
        return static_cast<int>(pages.size());
    }

    size_t getPageStartOffset(int index) const {
        const SplitPage* page = getPage(index);
        return page ? page->startOffset : 0;
    }

private:
    const SplitPage* getPage(int index) const {
        if (index < 0) return nullptr;

        const int frontCount = static_cast<int>(m_frontPages.size());
        if (index < frontCount) {
            return &m_frontPages[index];
        }

        int mainIndex = index - frontCount;
        if (mainIndex >= 0 && mainIndex < static_cast<int>(m_mainPages.size())) {
            return &m_mainPages[mainIndex];
        }
        return nullptr;
    }

    std::vector<SplitPage> m_frontPages;
    std::vector<SplitPage> m_mainPages;
    float m_fontSize = 18.0f;
};

ReaderPage::Ptr ReaderPage::create(const std::string& bookTitle, const std::string& content) {
    auto page = std::make_shared<ReaderPage>();
    page->initLayout(bookTitle, content);
    return page;
}

ReaderPage::ReaderPage() {
    setBackgroundColor(0xFFF6F1E1);
    m_progressStore = &InMemoryReadingProgressStore::getInstance();
}

void ReaderPage::onDisappear() {
    persistCurrentProgress();
    LFPage::onDisappear();
}

void ReaderPage::onExit() {
    persistCurrentProgress();
    LFPage::onExit();
}

void ReaderPage::initLayout(const std::string& title, const std::string& content) {
    m_bookId = title;

    auto root = LFBox::create();
    root->matchParentWidth();
    root->matchParentHeight();

    m_pageView = LFPageView::create();
    m_pageView->matchParentWidth();
    m_pageView->matchParentHeight();

    std::weak_ptr<ReaderPage> weakSelf = std::static_pointer_cast<ReaderPage>(shared_from_this());
    m_pageView->setOnTap([weakSelf](const LFPoint&) {
        if (auto self = weakSelf.lock()) {
            self->toggleMenu();
        }
    });
    m_pageView->setOnPageChangeListener([weakSelf](int index) {
        if (auto self = weakSelf.lock()) {
            self->onPageChanged(index);
        }
    });

    root->addChild(m_pageView, LFBoxAlign::MatchParent);

    setupTopBar(title);
    root->addChild(m_topBar, LFBoxAlign::TopLeft);

    addChild(root);
    splitContentAndInit(content);
}

size_t ReaderPage::sanitizeResumeOffset(size_t rawOffset) const {
    if (m_fullContent.empty()) return 0;
    size_t offset = std::min(rawOffset, m_fullContent.size() - 1);

    // 保护 UTF-8 边界：如果落在 continuation byte，则回退到字符头
    while (offset > 0) {
        unsigned char c = static_cast<unsigned char>(m_fullContent[offset]);
        if ((c & 0xC0) != 0x80) break;
        --offset;
    }
    return offset;
}

void ReaderPage::splitContentAndInit(const std::string& rawContent) {
    m_fullContent = normalizeText(rawContent);
    if (m_fullContent.empty()) return;

    const float fontSize = 18.0f;
    const float lineHeightScale = 1.6f;

    float windowW = LFEngine::getInstance().getWindowWidth();
    float windowH = LFEngine::getInstance().getWindowHeight();

    const float widthTolerance = 5.0f;
    float safeWidth = windowW - (ADAPTER_PADDING_X * 2.0f) - widthTolerance;
    if (safeWidth <= 0) safeWidth = 100.0f;

    float occupiedHeight = ADAPTER_PADDING_TOP + ADAPTER_PADDING_BOTTOM;
    float singleLineHeight = fontSize * lineHeightScale;
    float safetyBuffer = 5.0f;
    float safeHeight = windowH - occupiedHeight - safetyBuffer;
    if (safeHeight < singleLineHeight) safeHeight = singleLineHeight;

    m_splitConfig.width = safeWidth;
    m_splitConfig.height = safeHeight;
    m_splitConfig.fontSize = fontSize;
    m_splitConfig.lineHeight = lineHeightScale;
    m_splitConfig.fontFamily = "sans";

    const char* base = m_fullContent.c_str();
    const char* end = base + m_fullContent.size();

    size_t resumeOffset = 0;
    ReadingProgress progress;
    if (m_progressStore && m_progressStore->get(m_bookId, progress)) {
        resumeOffset = sanitizeResumeOffset(progress.pageStartOffset);
    }
    const char* anchor = base + resumeOffset;

    m_forwardIter.basePos = base;
    m_forwardIter.currentPos = anchor;
    m_forwardIter.endPos = end;
    m_forwardIter.isFinished = (anchor >= end);

    m_frontCursorOffset = resumeOffset;

    auto firstBatch = TextSplitter::splitStep(m_forwardIter, m_splitConfig, 10);

    // 兜底：若恢复点不可用，退回书籍开头
    if (firstBatch.empty()) {
        m_forwardIter.currentPos = base;
        m_forwardIter.endPos = end;
        m_forwardIter.isFinished = (base >= end);
        m_frontCursorOffset = 0;
        firstBatch = TextSplitter::splitStep(m_forwardIter, m_splitConfig, 10);
    }

    if (firstBatch.empty()) return;

    auto adapter = std::make_shared<BookPageAdapter>(firstBatch, fontSize);
    m_adapter = adapter;
    m_pageView->setAdapter(m_adapter);
    m_pageView->setCurrentItem(0, false);

    std::weak_ptr<ReaderPage> weakSelf = std::static_pointer_cast<ReaderPage>(shared_from_this());

    // 后向加载：从恢复页向书尾增量加载
    LFEngine::getInstance().addFrameTask([weakSelf]() mutable -> bool {
        auto self = weakSelf.lock();
        if (!self) return false;

        if (self->m_forwardIter.isFinished) return false;

        auto adapterPtr = std::dynamic_pointer_cast<BookPageAdapter>(self->m_adapter);
        if (!adapterPtr) return false;

        auto batch = TextSplitter::splitStep(self->m_forwardIter, self->m_splitConfig, 20);
        if (!batch.empty()) {
            adapterPtr->appendMainPages(batch);
            self->m_pageView->notifyDataSetChanged();
        }
        return !self->m_forwardIter.isFinished;
    });

    // 前向补齐：从书头向恢复页前增量加载
    LFEngine::getInstance().addFrameTask([weakSelf]() mutable -> bool {
        auto self = weakSelf.lock();
        if (!self) return false;

        if (self->m_frontCursorOffset == 0) return false;

        auto adapterPtr = std::dynamic_pointer_cast<BookPageAdapter>(self->m_adapter);
        if (!adapterPtr) return false;

        const char* base = self->m_fullContent.c_str();
        if (!base) return false;

        // 从恢复点向前分块回溯，优先补齐“当前页之前”最近的内容
        static const size_t WINDOW_BYTES = 64 * 1024;
        size_t scanStartOffset = 0;
        if (self->m_frontCursorOffset > WINDOW_BYTES) {
            scanStartOffset = self->m_frontCursorOffset - WINDOW_BYTES;
        }

        SplitIterator windowIter;
        windowIter.basePos = base;
        windowIter.currentPos = base + scanStartOffset;
        windowIter.endPos = base + self->m_frontCursorOffset;
        windowIter.isFinished = (windowIter.currentPos >= windowIter.endPos);

        std::vector<SplitPage> windowPages;
        while (!windowIter.isFinished) {
            auto chunk = TextSplitter::splitStep(windowIter, self->m_splitConfig, 200);
            if (chunk.empty()) break;
            windowPages.insert(windowPages.end(), chunk.begin(), chunk.end());
        }

        if (!windowPages.empty()) {
            int currentIndex = self->m_pageView->getCurrentItem();
            int added = adapterPtr->prependFrontPages(windowPages);

            self->m_pageView->notifyDataSetChanged();
            self->m_pageView->setCurrentItem(currentIndex + added, false);

            // 下一轮继续向更早位置回溯
            self->m_frontCursorOffset = windowPages.front().startOffset;
        } else {
            // 当前窗口无法再切分时，直接跳到窗口起点，避免卡死循环
            self->m_frontCursorOffset = scanStartOffset;
        }

        return self->m_frontCursorOffset > 0;
    });

    // 初始化时立即写入一次（用于首次打开后快速退出）
    persistCurrentProgress();
}

void ReaderPage::onPageChanged(int) {
    persistCurrentProgress();
}

void ReaderPage::persistCurrentProgress() {
    if (!m_progressStore || !m_pageView || m_bookId.empty()) return;

    auto adapter = std::dynamic_pointer_cast<BookPageAdapter>(m_adapter);
    if (!adapter) return;

    int currentIndex = m_pageView->getCurrentItem();
    ReadingProgress progress;
    progress.pageIndex = currentIndex;
    progress.pageStartOffset = adapter->getPageStartOffset(currentIndex);
    progress.updatedAt = LFEngine::getInstance().getElapsedTime();
    m_progressStore->put(m_bookId, progress);
}

void ReaderPage::setupTopBar(const std::string& title) {
    m_topBar = LFLinear::createHorizontal();
    m_topBar->matchParentWidth();
    m_topBar->setHeight(TOP_BAR_HEIGHT);
    m_topBar->setBackgroundColor(0xFFFFFFFF);
    m_topBar->setGravity(LFAlignment::Start, LFAlignment::Center);
    m_topBar->setPadding(YGEdgeHorizontal, 10.0f);
    m_topBar->setShadow(0, 2, 8, 0, 0x1A000000);

    std::weak_ptr<ReaderPage> weakSelf = std::static_pointer_cast<ReaderPage>(shared_from_this());

    auto backBtn = LFButton::create();
    backBtn->setWidth(44);
    backBtn->setHeight(44);

    auto backText = std::make_shared<LFText>();
    backText->setText("<");
    backText->setFontSize(24);
    backText->setTextColor(0xFF333333);
    backBtn->addChild(backText, LFBoxAlign::Center);

    backBtn->setOnClick([weakSelf](LFButton*) {
        if (auto self = weakSelf.lock()) {
            if (auto nav = self->getNavigator()) {
                nav->pop();
            }
        }
    });

    auto titleText = std::make_shared<LFText>();
    titleText->setText(title);
    titleText->setFontSize(16);
    titleText->setTextColor(0xFF333333);

    m_topBar->addChild(backBtn);
    m_topBar->addChild(titleText);
    m_isMenuVisible = true;
}

void ReaderPage::toggleMenu() {
    m_isMenuVisible = !m_isMenuVisible;
    if (!m_topBar) return;

    float startY = m_topBar->getTranslateY();
    float endY = m_isMenuVisible ? 0.0f : -60.0f;

    auto anim = LFValueAnimator<float>::of(startY, endY);
    anim->setDuration(0.2f);
    anim->setEasing(LFEasingType::QuadOut);

    auto topBar = m_topBar;
    anim->addUpdateListener([topBar](float val) {
        topBar->setTranslate(0, val);
    });
    anim->start();
    LFGlobalAnimationManager::getInstance().addAnimator(anim);
}
