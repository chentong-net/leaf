//
// Created by Chen Tong on 2026/1/17.
//

#ifndef LEAF_LFRENDERNODE_H
#define LEAF_LFRENDERNODE_H

#include "LFDef.h"

class LFRenderNode : public std::enable_shared_from_this<LFRenderNode> {
public:
    using Ptr = std::shared_ptr<LFRenderNode>;

    LFRenderNode();
    virtual ~LFRenderNode();

    // 层级管理
    void addChild(const Ptr& child);
    void removeChild(const Ptr& child);
    void setParent(LFRenderNode* parent) { m_parent = parent; }
    const std::vector<Ptr>& getChildren() const { return m_children; }

    void setMasksToBounds(bool masks) { m_masksToBounds = masks; }

    // 布局控制
    void setWidth(float width);
    void setHeight(float height);
    void setFlexDirection(YGFlexDirection direction);
    void setPadding(YGEdge edge, float padding);
    void setMargin(YGEdge edge, float margin);
    void setJustifyContent(YGJustify justify);
    void setAlignItems(YGAlign align);
    void setPositionType(YGPositionType type);
    void setPosition(YGEdge edge, float position);

    // 生命周期
    // 标记布局为脏，会向上递归通知父节点
    void markDirty();

    // 执行布局计算 (通常只由 Root 节点调用)
    void calculateLayout(float ownerWidth, float ownerHeight);

    // 获取 Yoga 计算后的结果
    float getLayoutX() const { return YGNodeLayoutGetLeft(m_ygNode); }
    float getLayoutY() const { return YGNodeLayoutGetTop(m_ygNode); }
    float getLayoutWidth() const { return YGNodeLayoutGetWidth(m_ygNode); }
    float getLayoutHeight() const { return YGNodeLayoutGetHeight(m_ygNode); }

    // 递归绘制
    virtual void render(NVGcontext* vg);

    // --- 关联引用 ---
    // 存储对应的 JS 对象引用，方便回调
    void setJSObject(JSValue obj) { m_jsObj = obj; }
    JSValue getJSObject() const { return m_jsObj; }

protected:
    // 子类必须实现的绘制逻辑
    virtual void onDraw(NVGcontext* vg) = 0;

    static NVGcolor colorToNVG(uint32_t argb);

    bool m_masksToBounds = false;

    YGNodeRef m_ygNode;
    LFRenderNode* m_parent = nullptr;
    std::vector<Ptr> m_children;

    // 状态位
    bool m_isDirty = true;

    // 绑定的 JS 对象（注意：在实际生产中需配合 QuickJS 引用计数管理）
    JSValue m_jsObj = JS_UNDEFINED;
};

#endif //LEAF_LFRENDERNODE_H
