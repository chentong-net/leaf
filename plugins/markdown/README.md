# Leaf Markdown

`LFMarkdownView` 是使用现有 Leaf UI 组件组合实现的跨平台 Markdown 渲染组件。

组件不直接调用 NanoVG、OpenGL 或 Metal，也不使用 `LFTexture`。文本、图片、表格、引用和列表分别由 `LFText`、`LFImage`、`LFBox`、`LFLinear` 与 `LFScrollView` 组合而成。

## 使用方式

```cpp
#include "LFMarkdown.h"

auto view = LFMarkdownView::create();
view->matchParentWidth();
view->matchParentHeight();
view->setOnLinkTap([](const std::string& destination) {
    LF_LOGI("open: %s", destination.c_str());
});
view->setMarkdown(source);
page->addChild(view);
```

主题通过 `LFMarkdownTheme` 配置。粗体使用 `LFFontWeight` 选择的真实内置字体；默认选择 `LFFontWeight::Bold`。由于当前资源没有 Italic 字体，斜体使用 `LFText` 的组件内倾斜效果。

## 支持范围

- ATX 与 Setext 标题
- 段落、软换行和硬换行
- 独立的粗体与斜体语法
- 链接点击和鼠标悬停下划线
- 行内代码与围栏代码块
- 行内图片与独立图片
- 有序列表、无序列表和嵌套列表
- 表格及左、中、右列对齐
- 多层嵌套引用
- 分隔线与反斜杠转义

当前不解析 HTML，HTML 标签会作为普通文本显示。
