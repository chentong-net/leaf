//
// Created by Chen Tong on 2026/1/17.
//

#ifndef LEAF_LFNODE_H
#define LEAF_LFNODE_H

#include "LFDef.h"

// 预声明
class LFNode;

// 变换属性结构体
struct LFTransform {
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float rotate = 0.0f; // 角度制
    float translateX = 0.0f;
    float translateY = 0.0f;
};

// 阴影属性结构体
struct LFShadow {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float blurRadius = 0.0f;
    float spread = 0.0f;
    uint32_t color = 0x00000000;
};

class LFNode : public std::enable_shared_from_this<LFNode> {
public:
    using Ptr = std::shared_ptr<LFNode>;

    LFNode();
    virtual ~LFNode();

    // ==========================================
    // 1. 树形结构管理 (Hierarchy)
    // ==========================================
    void addChild(const Ptr& child);
    void insertChild(const Ptr& child, uint32_t index);
    void removeChild(const Ptr& child);
    void removeFromParent();
    const std::vector<Ptr>& getChildren() const { return m_children; }
    LFNode* getParent() const { return m_parent; }

    // ==========================================
    // 2. 布局属性 (Layout - Yoga Proxy)
    // ==========================================
    // 尺寸与约束
    void setWidth(float width);
    void setHeight(float height);
    void setMinWidth(float minWidth);
    void setMaxWidth(float maxWidth);
    void setMinHeight(float minHeight);
    void setMaxHeight(float maxHeight);
    void setAspectRatio(float aspectRatio);

    // Flexbox 核心
    void setFlexDirection(YGFlexDirection direction);
    void setJustifyContent(YGJustify justify);
    void setAlignItems(YGAlign align);
    void setAlignSelf(YGAlign align);
    void setAlignContent(YGAlign align);
    void setFlexWrap(YGWrap wrap);
    void setFlexGrow(float grow);
    void setFlexShrink(float shrink);
    void setFlexBasis(float basis);

    // 间距
    void setPadding(YGEdge edge, float padding);
    void setMargin(YGEdge edge, float margin);
    void setBorderWidth(YGEdge edge, float width); // 布局层面的边框占位

    // 定位
    void setPositionType(YGPositionType type);
    void setPosition(YGEdge edge, float value);

    // 显示控制
    void setDisplay(YGDisplay display); // Flex 或 None (隐藏且不占位)

    // 方向
    void setDirection(YGDirection direction); // LTR 或 RTL

    // ==========================================
    // 3. 视觉样式属性 (Style & Visual)
    // ==========================================
    // 基础
    void setBackgroundColor(uint32_t color);
    void setOpacity(float opacity); // 0.0 ~ 1.0
    void setVisible(bool visible);  // true: 显示, false: 隐藏但占位 (visibility: hidden)

    // 边框与圆角
    void setBorderColor(uint32_t color);
    void setBorderRadius(float radius); // 统一圆角
    // void setBorderRadius(int corner, float radius); // 可扩展：单独设置四个角

    // 阴影 (Box Shadow)
    void setShadow(float offsetX, float offsetY, float blur, float spread, uint32_t color);

    // 变换 (Transform)
    void setScale(float x, float y);
    void setRotate(float angle); // 角度
    void setTranslate(float x, float y);

    // 裁剪
    void setMasksToBounds(bool masks); // overflow: hidden

    // ==========================================
    // 4. 引擎管线 (Pipeline)
    // ==========================================
    // 标记需要重排或重绘
    void markDirty();

    // 布局计算入口 (通常只由 Root 调用)
    void calculateLayout(float ownerWidth, float ownerHeight);

    // 渲染入口 (模板方法)
    void render(NVGcontext* vg);

    // 供高级子类使用 (如 Text 需要测量)
    YGNodeRef getYGNode() const { return m_ygNode; }

protected:
    // 子类扩展点：绘制具体内容 (Text, Image)
    // 内容绘制在背景之上，子视图之下
    virtual void onDrawContent(NVGcontext* vg) {}
    // 用于 YGNodeSetMeasureFunc 回调
    virtual YGSize measure(YGNodeRef node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) { return {0, 0}; }

    // 获取布局结果
    float getLayoutX() const { return YGNodeLayoutGetLeft(m_ygNode); }
    float getLayoutY() const { return YGNodeLayoutGetTop(m_ygNode); }
    float getLayoutWidth() const { return YGNodeLayoutGetWidth(m_ygNode); }
    float getLayoutHeight() const { return YGNodeLayoutGetHeight(m_ygNode); }

private:
    // 内部绘制实现
    void drawShadow(NVGcontext* vg, float w, float h);
    void drawBackground(NVGcontext* vg, float w, float h);
    void drawBorder(NVGcontext* vg, float w, float h);
    static NVGcolor colorToNVG(uint32_t argb);

    // Core Data
    YGNodeRef m_ygNode;
    LFNode* m_parent = nullptr;
    std::vector<Ptr> m_children;
    bool m_isDirty = true;

    // Style Data
    uint32_t m_backgroundColor = 0x00000000;
    uint32_t m_borderColor = 0x00000000;
    float m_borderRadius = 0.0f;
    float m_opacity = 1.0f;
    bool m_visible = true;
    bool m_masksToBounds = false;

    LFShadow m_shadow;
    LFTransform m_transform;
};

#endif // LEAF_LFNODE_H
