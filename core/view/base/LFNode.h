//
// Created by Chen Tong on 2026/1/17.
//

#ifndef LEAF_LFNODE_H
#define LEAF_LFNODE_H

#include "LFDef.h"
#include <functional>

// Forward declarations
class LFTouchEvent;
class LFNode;
class LFGestureRecognizer;
struct LFPoint;

// 变换属性结构体
struct LFTransform {
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float rotate = 0.0f; // 角度制
    float translateX = 0.0f;
    float translateY = 0.0f;
    float translatePercentX = 0.0f;
    float translatePercentY = 0.0f;
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

    // Event listener types
    using TouchEventListener = std::function<void(const LFTouchEvent&)>;
    using InterceptEventListener = std::function<bool(const LFTouchEvent&)>;

    LFNode();
    virtual ~LFNode();

    // 树操作
    void addChild(const Ptr& child);
    void insertChild(const Ptr& child, uint32_t index);
    void removeChild(const Ptr& child);
    void removeFromParent();
    const std::vector<Ptr>& getChildren() const { return m_children; }
    LFNode* getParent() const { return m_parent; }

    // 布局属性
    void setWidth(float width);
    void setHeight(float height);
    void setWidthPercent(float percent);
    void setHeightPercent(float percent);
    void setMinWidth(float minWidth);
    void setMaxWidth(float maxWidth);
    void setMinHeight(float minHeight);
    void setMaxHeight(float maxHeight);
    void setAspectRatio(float aspectRatio);

    void matchParentWidth();
    void matchParentHeight();
    void wrapContentWidth();
    void wrapContentHeight();

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

    void setPadding(YGEdge edge, float padding);
    void setMargin(YGEdge edge, float margin);

    // 定位
    void setPositionType(YGPositionType type);
    void setPosition(YGEdge edge, float value);
    void setPositionPercent(YGEdge edge, float percent);

    // 显示/隐藏
    void setDisplay(YGDisplay display);
    void setDirection(YGDirection direction);

    // 样式属性
    void setBackgroundColor(uint32_t color);
    void setOpacity(float opacity);
    void setVisible(bool visible);

    // 边框
    void setBorder(float width, uint32_t color);
    void setBorderRadius(float radius);

    // 阴影
    void setShadow(float offsetX, float offsetY, float blur, float spread, uint32_t color);
    void setRadius(float radius);

    // 变换
    void setScale(float x, float y);
    void setRotate(float angle); // 角度
    void setTranslate(float x, float y);
    void setTranslatePercent(float xPercent, float yPercent);

    // 裁剪
    void setMasksToBounds(bool masks); // overflow: hidden

    // 核心管线
    void markDirty(); // 脏标记
    void calculateLayout(float ownerWidth, float ownerHeight); // 布局
    void render(NVGcontext* vg); // 渲染

    // 供子类使用
    YGNodeRef getYGNode() const { return m_ygNode; }
    // 获取布局结果
    float getLayoutX() const { return YGNodeLayoutGetLeft(m_ygNode); }
    float getLayoutY() const { return YGNodeLayoutGetTop(m_ygNode); }
    float getLayoutWidth() const { return YGNodeLayoutGetWidth(m_ygNode); }
    float getLayoutHeight() const { return YGNodeLayoutGetHeight(m_ygNode); }
    float getRadius();
    float getTranslateX() const { return m_transform.translateX; }
    float getTranslateY() const { return m_transform.translateY; }
    float getScaleX() const { return m_transform.scaleX; }
    float getScaleY() const { return m_transform.scaleY; }

    // 获取变换（供 HitTest 使用）
    const LFTransform& getTransform() const { return m_transform; }

    // 可见性检查
    bool isVisible() const { return m_visible; }
    float getOpacity() const { return m_opacity; }

    uint32_t getBackgroundColor() const { return m_backgroundColor; }

    // Touch/HitTest 控制（事件系统扩展）
    void setTouchEnabled(bool enabled) { m_touchEnabled = enabled; }
    bool isTouchEnabled() const { return m_touchEnabled; }
    void setHitTestEnabled(bool enabled) { m_hitTestEnabled = enabled; }
    bool isHitTestEnabled() const { return m_hitTestEnabled; }

    // Event listeners (阶段1：基础触摸事件)
    void setOnTouchDown(TouchEventListener listener);
    void setOnTouchMove(TouchEventListener listener);
    void setOnTouchUp(TouchEventListener listener);
    void setOnTouchCancel(TouchEventListener listener);
    void setOnInterceptTouchEvent(InterceptEventListener listener);

    // Internal: Get event listeners (used by EventDispatcher)
    TouchEventListener getOnTouchDown() const { return m_onTouchDown; }
    TouchEventListener getOnTouchMove() const { return m_onTouchMove; }
    TouchEventListener getOnTouchUp() const { return m_onTouchUp; }
    TouchEventListener getOnTouchCancel() const { return m_onTouchCancel; }
    InterceptEventListener getOnInterceptTouchEvent() const { return m_onInterceptTouchEvent; }

    // ==========================================
    // Gesture Recognizers (阶段2：手势识别器)
    // ==========================================

    // Gesture callback types
    using TapCallback = std::function<void(const LFPoint&)>;
    using LongPressCallback = std::function<void(const LFPoint&)>;
    using PanStartCallback = std::function<void(const LFPoint& delta, const LFPoint& velocity)>;
    using PanUpdateCallback = std::function<void(const LFPoint& delta, const LFPoint& velocity)>;
    using PanEndCallback = std::function<void(const LFPoint& delta, const LFPoint& velocity)>;
    using PinchCallback = std::function<void(float scale, const LFPoint& focal)>;
    using RotateCallback = std::function<void(float angle, const LFPoint& focal)>;
    using SwipeCallback = std::function<void(int direction, const LFPoint& velocity)>;

    // Gesture recognizer management
    void addGestureRecognizer(std::shared_ptr<LFGestureRecognizer> recognizer);
    void removeGestureRecognizer(std::shared_ptr<LFGestureRecognizer> recognizer);
    void clearGestureRecognizers();
    const std::vector<std::shared_ptr<LFGestureRecognizer>>& getGestureRecognizers() const { return m_gestureRecognizers; }

    // Convenience methods for common gestures
    void setOnTap(TapCallback callback);
    void setOnDoubleTap(TapCallback callback);
    void setOnLongPress(LongPressCallback callback);
    void setOnPan(PanUpdateCallback onUpdate,
                  PanStartCallback onStart = nullptr,
                  PanEndCallback onEnd = nullptr);
    void setOnPinch(PinchCallback onUpdate,
                    PinchCallback onStart = nullptr,
                    PinchCallback onEnd = nullptr);
    void setOnRotate(RotateCallback onUpdate,
                     RotateCallback onStart = nullptr,
                     RotateCallback onEnd = nullptr);
    void setOnSwipe(SwipeCallback callback, int allowedDirections = 15); // 15 = Any direction

    static NVGcolor colorToNVG(uint32_t argb);

protected:
    // 布局前钩子，允许子类在正式布局前做尺寸准备
    virtual void onBeforeCalculateLayout(float ownerWidth, float ownerHeight) {}

    // 布局后钩子，允许子类恢复临时样式
    virtual void onAfterCalculateLayout() {}

    // 子类实现具体内容绘制
    // 内容绘制在背景之上，子视图之下
    virtual void onDrawContent(NVGcontext* vg) {}

    // 子类实现顶层绘制 (如滚动条、角标)
    // 绘制在子视图和边框之上
    virtual void onDrawOverlay(NVGcontext* vg) {}

private:
    void prepareLayoutTree(float ownerWidth, float ownerHeight);
    void finalizeLayoutTree();

    // 内部绘制实现
    void drawShadow(NVGcontext* vg, float w, float h);
    void drawBackground(NVGcontext* vg, float w, float h);
    void drawBorder(NVGcontext* vg, float w, float h);

    // Core Data
    YGNodeRef m_ygNode;
    LFNode* m_parent = nullptr;
    std::vector<Ptr> m_children;
    bool m_isDirty = true;

    // 样式数据
    uint32_t m_backgroundColor = 0x00000000;
    float m_opacity = 1.0f;
    bool m_visible = true;
    bool m_masksToBounds = false;
    float m_borderWidth = 0.0f;
    uint32_t m_borderColor = 0x00000000;
    float m_borderRadius = 0.0f;

    LFShadow m_shadow;
    LFTransform m_transform;

    // Event system
    bool m_touchEnabled = true;      // Can receive touch events
    bool m_hitTestEnabled = true;    // Participate in HitTest

    // Event listeners
    TouchEventListener m_onTouchDown;
    TouchEventListener m_onTouchMove;
    TouchEventListener m_onTouchUp;
    TouchEventListener m_onTouchCancel;
    InterceptEventListener m_onInterceptTouchEvent;

    // Gesture recognizers
    std::vector<std::shared_ptr<LFGestureRecognizer>> m_gestureRecognizers;
};

#endif // LEAF_LFNODE_H
