//
// Created by Chen Tong on 2026/1/31.
//

#include "LFGrid.h"
#include "LFBox.h"

LFGrid::Ptr LFGrid::create(int columnCount, float spacing) {
    auto grid = std::make_shared<LFGrid>();
    grid->setColumnCount(columnCount);
    grid->setSpacing(spacing);
    return grid;
}

LFGrid::LFGrid() {
    // 1. 强制为水平方向，这样子元素排满一行后会自动换到下一行
    setOrientation(LFOrientation::Horizontal);

    // 2. 开启自动换行 (关键)
    setFlexWrap(YGWrapWrap);

    // 3. 默认主轴对齐方式：起点对齐
    setGravity(LFAlignment::Start, LFAlignment::Start);

    // 4. 默认撑满父容器宽度，高度自适应
    matchParentWidth();
    wrapContentHeight();
}

void LFGrid::addChild(const LFNode::Ptr& child) {
    if (!child) return;

    // --- 代理单元格 (Cell Wrapper) 策略 ---

    // 1. 创建一个透明的 Box 作为 Wrapper
    auto cell = LFLinear::createVertical();

    // 2. 设置 Cell 的宽度百分比 = 100 / 列数
    // 这样无论屏幕多宽，一行永远正好放下 m_columnCount 个
    float widthPercent = 100.0f / (float)m_columnCount;
    cell->setWidthPercent(widthPercent);

    // 3. 纵向高度：默认包裹内容
    cell->wrapContentHeight();

    // 4. 设置内边距 (Padding) 来形成视觉间距
    // 举例：如果 spacing 是 10，那么每个 cell 四周都有 5 的 padding。
    // 相邻两个 cell 中间就是 10。边缘处会有 5 的留白。
    float halfSpacing = m_spacing / 2.0f;
    cell->setPadding(YGEdgeAll, halfSpacing);

    // 5. 将用户的 child 放入 cell
    // 用户的 child 应该撑满 cell 的内容区 (减去 padding 后的区域)
    // 除非用户显式设置了固定大小，否则我们默认让它撑满宽度
    child->matchParentWidth();

    // 如果用户没有设置高度，我们让它包裹内容；如果设置了，它会生效
    // 这里不做强制高度设置，尊重 child 原有属性

    cell->addChild(child); // 默认左上角

    // 6. 记录映射关系，以便后续 remove
    m_wrapperMap[child] = cell;

    // 7. 将 cell 添加到 Grid (LFLinear) 中
    LFLinear::addChild(cell);
}

void LFGrid::removeChild(const LFNode::Ptr& child) {
    // 1. 查找对应的 wrapper
    auto it = m_wrapperMap.find(child);
    if (it != m_wrapperMap.end()) {
        auto cell = it->second;

        // 2. 从 Grid 移除 wrapper
        LFLinear::removeChild(cell);

        // 3. 清理映射
        m_wrapperMap.erase(it);
    } else {
        // 如果没找到 wrapper (理论上不应该发生)，尝试直接移除
        LFLinear::removeChild(child);
    }
}

void LFGrid::setColumnCount(int count) {
    if (count < 1) count = 1;
    if (m_columnCount == count) return;

    m_columnCount = count;
    refreshCells();
}

void LFGrid::setSpacing(float spacing) {
    if (spacing < 0) spacing = 0;
    if (m_spacing == spacing) return;

    m_spacing = spacing;
    refreshCells();
}

void LFGrid::refreshCells() {
    float widthPercent = 100.0f / (float)m_columnCount;
    float halfSpacing = m_spacing / 2.0f;

    // 遍历所有 wrapper (也就是 LFLinear 的直接子节点)
    // 注意：getChildren() 返回的是 LFNode::Ptr 的列表，这里它们都是 cell
    const auto& cells = getChildren();

    for (const auto& cell : cells) {
        // 1. 更新宽度
        cell->setWidthPercent(widthPercent);

        // 2. 更新间距 (Padding)
        cell->setPadding(YGEdgeAll, halfSpacing);
    }

    // 标记脏，触发重绘布局
    markDirty();
}
