//
// Created by Chen Tong on 2026/1/17.
//

#ifndef LEAF_LFNODE_H
#define LEAF_LFNODE_H

#include "LFDef.h"
#include "layout/LFLayout.h"
#include "render/LFPaint.h"
#include <functional>

// Forward declarations
class LFTouchEvent;
class LFScrollEvent;
class LFKeyEvent;
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
    using ScrollEventListener = std::function<void(LFScrollEvent&)>;
    using InterceptEventListener = std::function<bool(const LFTouchEvent&)>;
    using KeyEventListener = std::function<void(LFKeyEvent&)>;
    using FocusChangeListener = std::function<void(bool)>;
    using HoverEventListener = std::function<void(bool)>;

    LFNode();
    virtual ~LFNode();

    // 树操作
    virtual void addChild(const Ptr& child);
    void insertChild(const Ptr& child, uint32_t index);
    virtual void removeChild(const Ptr& child);
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
    void setFlexDirection(LFFlexDirection direction);
    void setJustifyContent(LFJustify justify);
    void setAlignItems(LFAlign align);
    void setAlignSelf(LFAlign align);
    void setAlignContent(LFAlign align);
    void setFlexWrap(LFWrap wrap);
    void setFlexGrow(float grow);
    void setFlexShrink(float shrink);
    void setFlexBasis(float basis);

    void setPadding(LFLayoutEdge edge, float padding);
    void setMargin(LFLayoutEdge edge, float margin);
    void setGap(LFGutter gutter, float gap);

    // 定位
    void setPositionType(LFPositionType type);
    void setPosition(LFLayoutEdge edge, float value);
    void setPositionPercent(LFLayoutEdge edge, float percent);

    // 显示/隐藏
    void setDisplay(LFDisplay display);
    void setDirection(LFDirection direction);

    // 样式属性
    void setBackgroundColor(uint32_t color);
    void setOpacity(float opacity);
    void setVisible(bool visible);

    // 边框
    void setBorder(float width, uint32_t color);
    void setLayoutBorderWidth(LFLayoutEdge edge, float width);
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
    void render(LFPaint& paint); // 渲染

    const LFLayoutStyle& getLayoutStyle() const { return m_layoutStyle; }

    // 获取布局结果
    const LFLayoutFrame& getLayoutFrame() const { return m_layout->getFrame(); }
    float getLayoutX() const { return getLayoutFrame().x; }
    float getLayoutY() const { return getLayoutFrame().y; }
    float getLayoutWidth() const { return getLayoutFrame().width; }
    float getLayoutHeight() const { return getLayoutFrame().height; }
    float getRadius();
    float getTranslateX() const { return m_transform.translateX; }
    float getTranslateY() const { return m_transform.translateY; }
    float getScaleX() const { return m_transform.scaleX; }
    float getScaleY() const { return m_transform.scaleY; }

    // 获取变换（供 HitTest 使用）
    const LFTransform& getTransform() const { return m_transform; }
    bool convertWindowPointToLocal(float windowX, float windowY,
                                   float& localX, float& localY) const;
    void convertLocalPointToWindow(float& localX, float& localY) const;
    bool containsWindowPoint(float windowX, float windowY) const;

    // 可见性检查
    bool isVisible() const { return m_visible; }
    float getOpacity() const { return m_opacity; }
    bool isMasksToBounds() const { return m_masksToBounds; }

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
    void setOnScroll(ScrollEventListener listener);
    void setOnInterceptTouchEvent(InterceptEventListener listener);
    void setOnKeyDown(KeyEventListener listener);
    void setOnKeyUp(KeyEventListener listener);
    void setOnCharInput(KeyEventListener listener);
    void setOnFocusChange(FocusChangeListener listener);
    void setOnHoverChange(HoverEventListener listener);

    void setFocusable(bool focusable) { m_focusable = focusable; }
    bool isFocusable() const { return m_focusable; }
    bool hasFocus() const { return m_hasFocus; }
    bool isHovered() const { return m_isHovered; }
    void requestFocus();
    void clearFocus();

    // Internal: Get event listeners (used by EventDispatcher)
    TouchEventListener getOnTouchDown() const { return m_onTouchDown; }
    TouchEventListener getOnTouchMove() const { return m_onTouchMove; }
    TouchEventListener getOnTouchUp() const { return m_onTouchUp; }
    TouchEventListener getOnTouchCancel() const { return m_onTouchCancel; }
    ScrollEventListener getOnScroll() const { return m_onScroll; }
    InterceptEventListener getOnInterceptTouchEvent() const { return m_onInterceptTouchEvent; }
    KeyEventListener getOnKeyDown() const { return m_onKeyDown; }
    KeyEventListener getOnKeyUp() const { return m_onKeyUp; }
    KeyEventListener getOnCharInput() const { return m_onCharInput; }
    HoverEventListener getOnHoverChange() const { return m_onHoverChange; }

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

protected:
    // 布局前钩子，允许子类在正式布局前做尺寸准备
    virtual void onBeforeCalculateLayout(float ownerWidth, float ownerHeight) {}

    // 布局后钩子，允许子类恢复临时样式
    virtual void onAfterCalculateLayout() {}

    virtual void onFocusChanged(bool focused) {}
    virtual void onHoverChanged(bool hovered) {}

    // 子类实现具体内容绘制
    // 内容绘制在背景之上，子视图之下
    virtual void onDrawContent(LFPaint& paint) {}

    // 子类实现顶层绘制 (如滚动条、角标)
    // 绘制在子视图和边框之上
    virtual void onDrawOverlay(LFPaint& paint) {}

    void setLayoutMeasureFunction(LFMeasureFunction function);
    void setLayoutBaselineFunction(LFBaselineFunction function);
    void invalidateLayout();

private:
    friend class LFEventDispatcher;
    void setFocusState(bool focused);
    void setHoverState(bool hovered);

    void prepareLayoutTree(float ownerWidth, float ownerHeight);
    void finalizeLayoutTree();
    void updateLayoutFrameTree();
    void applyLayoutStyle();

    // 内部绘制实现
    void drawShadow(LFPaint& paint, float w, float h);
    void drawBackground(LFPaint& paint, float w, float h);
    void drawBorder(LFPaint& paint, float w, float h);

    // Core Data
    LFLayoutStyle m_layoutStyle;
    std::unique_ptr<LFLayout> m_layout;
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
    bool m_focusable = false;
    bool m_hasFocus = false;
    bool m_isHovered = false;

    // Event listeners
    TouchEventListener m_onTouchDown;
    TouchEventListener m_onTouchMove;
    TouchEventListener m_onTouchUp;
    TouchEventListener m_onTouchCancel;
    ScrollEventListener m_onScroll;
    InterceptEventListener m_onInterceptTouchEvent;
    KeyEventListener m_onKeyDown;
    KeyEventListener m_onKeyUp;
    KeyEventListener m_onCharInput;
    FocusChangeListener m_onFocusChange;
    HoverEventListener m_onHoverChange;

    // Gesture recognizers
    std::vector<std::shared_ptr<LFGestureRecognizer>> m_gestureRecognizers;
};

#endif // LEAF_LFNODE_H
