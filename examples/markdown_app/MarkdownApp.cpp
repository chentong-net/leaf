#include "MarkdownApp.h"

#include "LFMarkdownView.h"

namespace {

constexpr const char* kShowcaseMarkdown =
    R"MARKDOWN(
# Markdown 综合渲染展示

> 用于展示 Markdown 渲染效果。

## 1. 标题与段落

### 三级标题
#### 四级标题
##### 五级标题
###### 六级标题

这是第一段正文。段落会自动换行，并保持与其他块级元素之间的间距。
这一行与上一行属于同一个段落，应当连续排版。

这是强制换行前的文字。\
这是强制换行后的文字。

这是使用两个空格产生的强制换行。)MARKDOWN"
    "  \n"
    R"MARKDOWN(这是第二个强制换行后的文字。

这是独立的新段落，包含中文标点：粗体、斜体、行内代码、链接和图片需要保持相同的基线。

## 2. 行内样式

普通文字；**中文粗体**；*中文斜体*；***中文粗斜体***；`const value = 42;`；[Leaf 链接](https://example.com/leaf)。

转义字符：\*这里不是斜体\*，\`这里不是代码\`，\[这里不是链接\]。

文本前后都有图片：![行内图片](avatar.jpg "Leaf avatar") 图片后继续显示文字。

The renderer should wrap a longer English paragraph without collapsing words. It should also handle
the quick brown fox jumps over the lazy dog.

## 3. 列表

- 无序列表第一项
- 无序列表第二项包含 **粗体**、*斜体*、`代码` 和 [链接](https://example.com/list)。
- 无序列表第三项用于检查多行内容，这段文字足够长，在窄屏设备上应当自然换行并保持悬挂缩进。
  - 二级无序项目
    1. 三级有序项目
    2. 三级有序项目的第二行
  - 二级无序项目的另一项

3. 有序列表从 3 开始
4. 第二项包含嵌套列表
   - 嵌套无序项目 A
   - 嵌套无序项目 B
5. 有序列表结束项

- 松散列表第一段

  这是同一个列表项中的第二段。
- 松散列表的下一项

## 4. 嵌套引用

> 第一层引用正文。
>
> 第一层引用中的第二个段落。
>
>> 第二层引用包含 **粗体文字** 和 `inline code`。
>>
>>> 第三层引用。
>>>
>>> - 引用中的无序列表
>>> - 引用中的第二个无序项目
>>>
>>> ```cpp
>>> auto quote = "nested code";
>>> ```

## 5. 表格

| 能力 | 桌面端状态 | iOS 状态 | 说明 |
| :--- | :---: | :---: | ---: |
| 标题与段落 | 已支持 | 已支持 | 自动换行 |
| 行内样式 | 粗体、斜体 | 代码、链接 | 检查单元格 |
| 长文本 | 这是一段较长的表格内容 | 窄屏需要换行 | 右对齐 |
| 空单元格 |  | 已检查 |  |

## 6. 图片

![独立图片](avatar.jpg "Leaf avatar")

## 7. 代码块

```cpp
auto view = LFMarkdownView::create();
view->setMarkdown(markdown);
view->setOnLinkTap([](const std::string& destination) {
    LF_LOGI("Markdown link: %s", destination.c_str());
});
page->addChild(view);
```

~~~text
围栏代码块可以保留连续空格：
    indented line
        nested indent
~~~

## 8. 边界语法

HTML 暂不解析，因此 <strong>这段内容会按原始文本显示</strong>。

未闭合标记按普通文字显示：**未闭合粗体 和 `未闭合代码。

这是一个很长的不可断开字符串：https://example.com/a/very/long/path/that/should/not/break/inside/flex/layout

---

测试结束。
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
