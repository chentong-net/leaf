#include "MarkdownApp.h"

#include "LFMarkdownView.h"

namespace {

constexpr const char* kShowcaseMarkdown = R"MARKDOWN(
# Leaf Markdown

一个完全由现有 Leaf 组件组合而成的 **跨平台 Markdown 渲染器**。支持 **粗体**、*斜体*、`行内代码` 和 [可点击链接](https://example.com/leaf)。

## 图片

![Leaf 示例图片](avatar.jpg "Leaf avatar")

## 列表

1. 第一项包含 **真实粗体**
2. 第二项包含 *斜体* 和 `inline code`
   - 嵌套无序项目
   - 另一个嵌套项目
3. 第三项用于展示有序列表编号

## 表格

| 能力 | 状态 | 说明 |
| :--- | :---: | ---: |
| 标题与段落 | 已支持 | 自动换行与间距 |
| 图片 | 已支持 | 使用 LFImage |
| 表格 | 已支持 | 列对齐与自适应宽度 |

## 嵌套引用

> 第一层引用。
>> 第二层引用，包含 **粗体文字**。
>>> 第三层引用。
>>>
>>> - 引用中的列表
>>> - 仍然可以继续嵌套块级内容

## 代码块

```cpp
auto view = LFMarkdownView::create();
view->setMarkdown(markdown);
page->addChild(view);
```

---

HTML 暂不解析，因此 `<strong>这段内容会按原始文本显示</strong>`。
)MARKDOWN";

} // namespace

MarkdownApp::Ptr MarkdownApp::create() {
    return std::make_shared<MarkdownApp>();
}

LFNode::Ptr MarkdownApp::start() {
    m_navigator = LFNavigator::create();

    auto page = LFPage::create();
    page->setBackgroundColor(0xFFF8FAFC);

    auto markdown = LFMarkdownView::create();
    markdown->matchParentWidth();
    markdown->matchParentHeight();
    markdown->setOnLinkTap([](const std::string& destination) {
        LF_LOGI("Markdown link: %s", destination.c_str());
    });
    markdown->setMarkdown(kShowcaseMarkdown);
    page->addChild(markdown);

    m_navigator->push(page, false);
    return m_navigator;
}
