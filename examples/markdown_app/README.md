# Markdown 示例应用

该示例展示 Markdown 插件的能力，包括：

- ATX 与 Setext 标题
- 软换行、反斜杠强制换行和双空格强制换行
- 使用真实内置 Bold 字重的粗体
- 独立斜体和行内代码
- 链接点击与悬停下划线
- 独立图片和保持原始比例的行内图片
- 有序、无序和嵌套列表
- 表格列对齐
- 多层嵌套引用
- 围栏代码块

当前应用入口会启动该示例。

## 测试场景

示例应用支持以下场景：

- `overview`：默认综合场景
- `blocks`：标题、段落、软换行、强制换行和分隔线
- `inline`：行内样式、代码、链接、转义和行内图片
- `list`：有序、无序、嵌套和松散列表
- `quote`：多层引用、引用列表和引用代码块
- `table`：列宽、对齐、长文本和空单元格
- `media`：独立图片与代码块
- `edge`：HTML 原样文本、未闭合标记、长链接和换行

macOS 可通过环境变量选择场景：

```bash
LEAF_MARKDOWN_SCENARIO=blocks ./leaf-macos
```

iOS 模拟器可通过 `SIMCTL_CHILD_` 向应用进程传递相同变量：

```bash
SIMCTL_CHILD_LEAF_MARKDOWN_SCENARIO=blocks \
  xcrun simctl launch --terminate-running-process booted <bundle-id>
```
