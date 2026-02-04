//
// Created by Chen Tong on 2026/1/30.
//

#ifndef LEAF_LFGRID_H
#define LEAF_LFGRID_H

#include "LFLinear.h"
#include <unordered_map>

/**
 * 网格布局容器 (Grid Layout)
 *
 * 基于 Flexbox Wrap 实现的固定列数网格。
 * 采用 "Cell Wrapper" 策略来实现完美的固定间距和自适应宽度。
 *
 * 特性：
 * 1. 自动换行
 * 2. 指定列数 (ColumnCount)
 * 3. 指定间距 (Spacing)
 * 4. 自动撑满父容器宽度
 */
class LFGrid : public LFLinear {
public:
    using Ptr = std::shared_ptr<LFGrid>;

    /**
     * 创建网格
     * @param columnCount 列数 (默认 3)
     * @param spacing 间距像素 (默认 0)
     */
    static Ptr create(int columnCount = 3, float spacing = 0.0f);

    LFGrid();
    virtual ~LFGrid() = default;

    /**
     * 重写 addChild
     * 会自动将 child 包装在一个透明的 Cell 中以控制布局
     */
    void addChild(const LFNode::Ptr& child);

    /**
     * 移除子节点
     * 会自动移除对应的 Cell 包装器
     */
    void removeChild(const LFNode::Ptr& child);

    /**
     * 设置列数
     * 修改后会自动刷新所有子节点的宽度
     */
    void setColumnCount(int count);

    /**
     * 设置间距 (行和列同时设置)
     */
    void setSpacing(float spacing);

    /**
     * 单独设置行间距 (目前方案统一控制 Padding，建议使用 setSpacing，
     * 如需单独控制需扩展 Cell 逻辑，这里暂且映射为统一间距或预留)
     */
    // void setRowSpacing(float spacing); // 暂略，为了保证完美对齐，建议统一间距

    /**
     * 获取列数
     */
    int getColumnCount() const { return m_columnCount; }

private:
    /**
     * 刷新所有 Cell 的样式 (宽度和 Padding)
     */
    void refreshCells();

    int m_columnCount = 3;
    float m_spacing = 0.0f;

    // 映射表：真实子节点 -> 代理 Cell 节点
    // 用于 removeChild 时能找到对应的 wrapper
    std::unordered_map<LFNode::Ptr, LFNode::Ptr> m_wrapperMap;
};

#endif // LEAF_LFGRID_H
