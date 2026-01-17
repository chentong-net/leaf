//
// Created by Chen Tong on 2026/1/17.
//

#include "LFRenderNode.h"
#include <algorithm>

LFRenderNode::LFRenderNode() {
    m_ygNode = YGNodeNew();
    YGNodeSetContext(m_ygNode, this);
}

LFRenderNode::~LFRenderNode() {
    if (m_ygNode) {
        YGNodeFree(m_ygNode);
    }
}

// 层级管理
void LFRenderNode::addChild(const Ptr& child) {
    child->setParent(this);
    m_children.push_back(child);
    // 同步到 Yoga 树
    YGNodeInsertChild(m_ygNode, child->m_ygNode, (uint32_t)YGNodeGetChildCount(m_ygNode));
    markDirty();
}

void LFRenderNode::removeChild(const Ptr& child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        YGNodeRemoveChild(m_ygNode, child->m_ygNode);
        child->setParent(nullptr);
        m_children.erase(it);
        markDirty();
    }
}

// 布局控制实现

void LFRenderNode::setWidth(float width) {
    if (width < 0) YGNodeStyleSetWidthAuto(m_ygNode); // 对应 Size.WRAP_CONTENT
    else YGNodeStyleSetWidth(m_ygNode, width);
    markDirty();
}

void LFRenderNode::setHeight(float height) {
    if (height < 0) YGNodeStyleSetHeightAuto(m_ygNode);
    else YGNodeStyleSetHeight(m_ygNode, height);
    markDirty();
}

void LFRenderNode::setFlexDirection(YGFlexDirection direction) {
    YGNodeStyleSetFlexDirection(m_ygNode, direction);
    markDirty();
}

void LFRenderNode::setPadding(YGEdge edge, float padding) {
    YGNodeStyleSetPadding(m_ygNode, edge, padding);
    markDirty();
}

void LFRenderNode::setMargin(YGEdge edge, float margin) {
    YGNodeStyleSetMargin(m_ygNode, edge, margin);
    markDirty();
}

void LFRenderNode::setJustifyContent(YGJustify justify) {
    YGNodeStyleSetJustifyContent(m_ygNode, justify);
    markDirty();
}

void LFRenderNode::setAlignItems(YGAlign align) {
    YGNodeStyleSetAlignItems(m_ygNode, align);
    markDirty();
}

void LFRenderNode::setPositionType(YGPositionType type) {
    YGNodeStyleSetPositionType(m_ygNode, type);
    markDirty();
}

void LFRenderNode::setPosition(YGEdge edge, float position) {
    YGNodeStyleSetPosition(m_ygNode, edge, position);
    markDirty();
}

// 生命周期

void LFRenderNode::markDirty() {
    m_isDirty = true;
    // 向上递归，告知父节点布局失效
    if (m_parent) {
        m_parent->markDirty();
    }
}

void LFRenderNode::calculateLayout(float ownerWidth, float ownerHeight) {
    if (m_isDirty) {
        // 执行真正的 Flexbox 数学计算
        YGNodeCalculateLayout(m_ygNode, ownerWidth, ownerHeight, YGDirectionLTR);
        m_isDirty = false;
    }
}

void LFRenderNode::render(NVGcontext* vg) {
    // 1. 获取 Yoga 计算出的相对父节点的坐标
    float x = getLayoutX();
    float y = getLayoutY();
    float w = getLayoutWidth();
    float h = getLayoutHeight();

    // 2. 状态入栈并平移坐标系
    nvgSave(vg);
    nvgTranslate(vg, x, y);

    if (m_masksToBounds) {
        // 告知 NanoVG，接下来的绘制只能在这个矩形框内显示
        nvgIntersectScissor(vg, 0, 0, w, h);
    }

    // 3. 执行绘制逻辑 (子类实现)
    onDraw(vg);

    // 4. 递归绘制子节点
    for (auto& child : m_children) {
        child->render(vg);
    }

    // 5. 状态出栈
    nvgRestore(vg);
}

NVGcolor LFRenderNode::colorToNVG(uint32_t argb) {
    unsigned char a = (argb >> 24) & 0xFF;
    unsigned char r = (argb >> 16) & 0xFF;
    unsigned char g = (argb >> 8) & 0xFF;
    unsigned char b = argb & 0xFF;
    return nvgRGBA(r, g, b, a);
}