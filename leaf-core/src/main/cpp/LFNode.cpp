//
// Created by Chen Tong on 2026/1/17.
//

#include "LFNode.h"

// 简化 setter 编写，只有值变了才 markDirty
#define SET_STYLE_VAL(prop, value) \
    if (prop == value) return; \
    prop = value; \
    markDirty();

#define SET_YOGA_VAL(func, ...) \
    func(m_ygNode, __VA_ARGS__); \
    markDirty();

LFNode::LFNode() {
    m_ygNode = YGNodeNew();
    YGNodeSetContext(m_ygNode, this);
    // 默认行为配置
    YGNodeStyleSetFlexDirection(m_ygNode, YGFlexDirectionColumn); // 默认垂直
    YGNodeStyleSetPositionType(m_ygNode, YGPositionTypeRelative);
}

LFNode::~LFNode() {
    if (m_ygNode) {
        YGNodeFree(m_ygNode);
    }
}

// ==========================================
// 树管理实现
// ==========================================
void LFNode::addChild(const Ptr& child) {
    if (child->getParent()) {
        child->removeFromParent();
    }
    child->m_parent = this;
    m_children.push_back(child);
    YGNodeInsertChild(m_ygNode, child->m_ygNode, YGNodeGetChildCount(m_ygNode));
    markDirty();
}

void LFNode::insertChild(const Ptr& child, uint32_t index) {
    if (child->getParent()) {
        child->removeFromParent();
    }
    if (index > m_children.size()) index = m_children.size();

    child->m_parent = this;
    m_children.insert(m_children.begin() + index, child);
    YGNodeInsertChild(m_ygNode, child->m_ygNode, index);
    markDirty();
}

void LFNode::removeChild(const Ptr& child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        YGNodeRemoveChild(m_ygNode, child->m_ygNode);
        child->m_parent = nullptr;
        m_children.erase(it);
        markDirty();
    }
}

void LFNode::removeFromParent() {
    if (m_parent) {
        // 创建临时智能指针防止自身析构 (shared_from_this)
        auto self = shared_from_this();
        m_parent->removeChild(self);
    }
}

// ==========================================
// 布局属性实现 (Yoga Proxy)
// ==========================================
void LFNode::setWidth(float width) {
    if (width < 0) {
        YGNodeStyleSetWidthAuto(m_ygNode);
        markDirty();
    } else {
        SET_YOGA_VAL(YGNodeStyleSetWidth, width)
    }
}
void LFNode::setHeight(float height) {
    if (height < 0) {
        YGNodeStyleSetHeightAuto(m_ygNode);
        markDirty();
    } else {
        SET_YOGA_VAL(YGNodeStyleSetHeight, height)
    }
}
void LFNode::setWidthPercent(float percent) {
    YGNodeStyleSetWidthPercent(m_ygNode, percent);
    markDirty();
}

void LFNode::setHeightPercent(float percent) {
    YGNodeStyleSetHeightPercent(m_ygNode, percent);
    markDirty();
}
void LFNode::setMinWidth(float minWidth) { SET_YOGA_VAL(YGNodeStyleSetMinWidth, minWidth); }
void LFNode::setMaxWidth(float maxWidth) { SET_YOGA_VAL(YGNodeStyleSetMaxWidth, maxWidth); }
void LFNode::setMinHeight(float minHeight) { SET_YOGA_VAL(YGNodeStyleSetMinHeight, minHeight); }
void LFNode::setMaxHeight(float maxHeight) { SET_YOGA_VAL(YGNodeStyleSetMaxHeight, maxHeight); }
void LFNode::setAspectRatio(float aspectRatio) { SET_YOGA_VAL(YGNodeStyleSetAspectRatio, aspectRatio); }

void LFNode::matchParentWidth() {
    setWidthPercent(100.0f);
}

void LFNode::matchParentHeight() {
    setHeightPercent(100.0f);
}

void LFNode::wrapContentWidth() {
    YGNodeStyleSetWidthAuto(m_ygNode);
    markDirty();
}

void LFNode::wrapContentHeight() {
    YGNodeStyleSetHeightAuto(m_ygNode);
    markDirty();
}

void LFNode::setFlexDirection(YGFlexDirection direction) { SET_YOGA_VAL(YGNodeStyleSetFlexDirection, direction); }
void LFNode::setJustifyContent(YGJustify justify) { SET_YOGA_VAL(YGNodeStyleSetJustifyContent, justify); }
void LFNode::setAlignItems(YGAlign align) { SET_YOGA_VAL(YGNodeStyleSetAlignItems, align); }
void LFNode::setAlignSelf(YGAlign align) { SET_YOGA_VAL(YGNodeStyleSetAlignSelf, align); }
void LFNode::setAlignContent(YGAlign align) { SET_YOGA_VAL(YGNodeStyleSetAlignContent, align); }
void LFNode::setFlexWrap(YGWrap wrap) { SET_YOGA_VAL(YGNodeStyleSetFlexWrap, wrap); }
void LFNode::setFlexGrow(float grow) { SET_YOGA_VAL(YGNodeStyleSetFlexGrow, grow); }
void LFNode::setFlexShrink(float shrink) { SET_YOGA_VAL(YGNodeStyleSetFlexShrink, shrink); }
void LFNode::setFlexBasis(float basis) { SET_YOGA_VAL(YGNodeStyleSetFlexBasis, basis); }

void LFNode::setPadding(YGEdge edge, float padding) { SET_YOGA_VAL(YGNodeStyleSetPadding, edge, padding); }
void LFNode::setMargin(YGEdge edge, float margin) { SET_YOGA_VAL(YGNodeStyleSetMargin, edge, margin); }

void LFNode::setPositionType(YGPositionType type) { SET_YOGA_VAL(YGNodeStyleSetPositionType, type); }
void LFNode::setPosition(YGEdge edge, float value) { SET_YOGA_VAL(YGNodeStyleSetPosition, edge, value); }
void LFNode::setPositionPercent(YGEdge edge, float percent) {
    YGNodeStyleSetPositionPercent(m_ygNode, edge, percent);
    markDirty();
}
void LFNode::setDisplay(YGDisplay display) { SET_YOGA_VAL(YGNodeStyleSetDisplay, display); }
void LFNode::setDirection(YGDirection direction) { SET_YOGA_VAL(YGNodeStyleSetDirection, direction); }

// ==========================================
// 样式属性实现
// ==========================================
void LFNode::setBackgroundColor(uint32_t color) { SET_STYLE_VAL(m_backgroundColor, color); }
void LFNode::setOpacity(float opacity) { SET_STYLE_VAL(m_opacity, std::clamp(opacity, 0.0f, 1.0f)); }
void LFNode::setVisible(bool visible) { SET_STYLE_VAL(m_visible, visible); }
void LFNode::setBorderRadius(float radius) { SET_STYLE_VAL(m_borderRadius, radius); }
void LFNode::setRadius(float radius) {
    setBorderRadius(radius);
}
float LFNode::getRadius() {
    return m_borderRadius;
}
void LFNode::setMasksToBounds(bool masks) { SET_STYLE_VAL(m_masksToBounds, masks); }

void LFNode::setBorder(float width, uint32_t color) {
    if (m_borderWidth == width && m_borderColor == color) return;

    m_borderWidth = std::max(0.0f, width);
    m_borderColor = color;

    // 同步告诉 Yoga 边框宽度
    // 这样 Yoga 在计算 content box 时会自动扣除边框厚度，符合标准盒模型
    YGNodeStyleSetBorder(m_ygNode, YGEdgeAll, m_borderWidth);

    markDirty();
}

void LFNode::setShadow(float offsetX, float offsetY, float blur, float spread, uint32_t color) {
    if (m_shadow.offsetX == offsetX && m_shadow.offsetY == offsetY &&
        m_shadow.blurRadius == blur && m_shadow.spread == spread &&
        m_shadow.color == color) return;

    m_shadow = {offsetX, offsetY, blur, spread, color};
    markDirty();
}

void LFNode::setScale(float x, float y) {
    if (m_transform.scaleX == x && m_transform.scaleY == y) return;
    m_transform.scaleX = x; m_transform.scaleY = y;
    markDirty();
}

void LFNode::setRotate(float angle) {
    if (m_transform.rotate == angle) return;
    m_transform.rotate = angle;
    markDirty();
}

void LFNode::setTranslate(float x, float y) {
    if (m_transform.translateX == x && m_transform.translateY == y) return;
    m_transform.translateX = x; m_transform.translateY = y;
    markDirty();
}

void LFNode::setTranslatePercent(float xPercent, float yPercent) {
    if (m_transform.translatePercentX == xPercent &&
        m_transform.translatePercentY == yPercent) return;

    m_transform.translatePercentX = xPercent;
    m_transform.translatePercentY = yPercent;
    markDirty();
}

// ==========================================
// 引擎管线核心
// ==========================================
void LFNode::markDirty() {
    m_isDirty = true;
    // 只有非 Root 节点才需要向上传递 Dirty
    // 实际上 Yoga 内部会自动标记父节点，这里主要为了自身的重绘逻辑
    if (m_parent) m_parent->markDirty();
}

void LFNode::calculateLayout(float ownerWidth, float ownerHeight) {
    // 布局
    // 注意：每次 update 都会调用，Yoga 内部有缓存机制优化
    YGNodeCalculateLayout(m_ygNode, ownerWidth, ownerHeight, YGDirectionLTR);
    m_isDirty = false;
}

/**
 * 核心渲染流程 (Template Method)
 */
void LFNode::render(NVGcontext* vg) {
    // 0. 基础过滤：如果隐藏或透明度为0，直接跳过
    // 注意：YGDisplayNone 的节点 Yoga 布局宽高会是 0，也会在这里被过滤
    if (!m_visible || m_opacity <= 0.0f) return;

    float w = getLayoutWidth();
    float h = getLayoutHeight();
    if (w <= 0 || h <= 0) return;

    // 1. 保存当前 GL 状态 (Transform, Alpha, Scissor 等)
    nvgSave(vg);

    // 2. 应用布局坐标 (Layout X/Y)
    float x = getLayoutX();
    float y = getLayoutY();
    nvgTranslate(vg, x, y);

    // 3. 应用视觉变换 (Transform: Translate -> Rotate -> Scale)
    // 变换原点默认中心
    float tx = m_transform.translateX;
    float ty = m_transform.translateY;
    tx += w * (m_transform.translatePercentX / 100.0f);
    ty += h * (m_transform.translatePercentY / 100.0f);
    if (tx != 0 || ty != 0) nvgTranslate(vg, tx, ty);

    if (m_transform.rotate != 0 || m_transform.scaleX != 1.0f || m_transform.scaleY != 1.0f) {
        float cx = w * 0.5f;
        float cy = h * 0.5f;
        nvgTranslate(vg, cx, cy);
        if (m_transform.rotate != 0) nvgRotate(vg, nvgDegToRad(m_transform.rotate));
        if (m_transform.scaleX != 1.0f || m_transform.scaleY != 1.0f) nvgScale(vg, m_transform.scaleX, m_transform.scaleY);
        nvgTranslate(vg, -cx, -cy);
    }

    // 4. 应用透明度
    if (m_opacity < 1.0f) {
        nvgGlobalAlpha(vg, m_opacity);
    }

    // 5. 绘制阴影 (在背景之前)
    drawShadow(vg, w, h);

    // 6. 绘制背景
    drawBackground(vg, w, h);

    // 8. 裁剪 (Overflow: Hidden)
    if (m_masksToBounds) {
        // IntersectScissor 是基于当前 Transform 的
        nvgIntersectScissor(vg, 0, 0, w, h);
    }

    // 7. 绘制内容 (子类如 Text/Image)
    onDrawContent(vg);

    // 9. 递归绘制子节点
    for (auto& child : m_children) {
        child->render(vg);
    }

    // 10. 绘制边框 (在子节点之上，防止被内容遮挡)
    drawBorder(vg, w, h);

    // 11. 恢复状态
    nvgRestore(vg);
}

// ==========================================
// 内部绘制实现
// ==========================================
void LFNode::drawShadow(NVGcontext* vg, float w, float h) {
    if ((m_shadow.color >> 24) & 0xFF) {
        // 阴影绘制比较复杂，通常使用 BoxGradient
        // 这里只是一个简单的实现思路
        float spread = m_shadow.spread;
        float blur = m_shadow.blurRadius;

        // 阴影矩形的位置
        float sx = m_shadow.offsetX - spread;
        float sy = m_shadow.offsetY - spread;
        float sw = w + spread * 2;
        float sh = h + spread * 2;
        float r = m_borderRadius + spread;

        NVGpaint shadowPaint = nvgBoxGradient(vg,
                                              m_shadow.offsetX, m_shadow.offsetY + 2, // 微调光照位置
                                              w, h,
                                              m_borderRadius, blur,
                                              colorToNVG(m_shadow.color), nvgRGBA(0,0,0,0));

        nvgBeginPath(vg);
        nvgRect(vg, sx - blur, sy - blur, sw + blur*2, sh + blur*2);
        // 使用 PathWinding 镂空中间部分，防止阴影叠加在背景下导致变色
        nvgRoundedRect(vg, 0, 0, w, h, m_borderRadius);
        nvgPathWinding(vg, NVG_HOLE);

        nvgFillPaint(vg, shadowPaint);
        nvgFill(vg);
    }
}

void LFNode::drawBackground(NVGcontext* vg, float w, float h) {
    if ((m_backgroundColor >> 24) & 0xFF) {
        nvgBeginPath(vg);
        if (m_borderRadius > 0) {
            nvgRoundedRect(vg, 0, 0, w, h, m_borderRadius);
        } else {
            nvgRect(vg, 0, 0, w, h);
        }
        nvgFillColor(vg, colorToNVG(m_backgroundColor));
        nvgFill(vg);
    }
}

void LFNode::drawBorder(NVGcontext* vg, float w, float h) {
    // 统一边框绘制
    // 只有当设置了边框宽且颜色不透明时才绘制
    if (m_borderWidth > 0.0f && ((m_borderColor >> 24) & 0xFF)) {
        // NanoVG 的 Stroke 是居中的。为了让边框完全在节点内(Border-Box模型)，
        // 我们需要向内缩半个边框宽度。
        float half = m_borderWidth * 0.5f;

        nvgBeginPath(vg);
        if (m_borderRadius > 0) {
            // 圆角也要相应收缩，防止内部圆角异常
            float innerRadius = std::max(0.0f, m_borderRadius - half);
            nvgRoundedRect(vg, half, half, w - m_borderWidth, h - m_borderWidth, innerRadius);
        } else {
            nvgRect(vg, half, half, w - m_borderWidth, h - m_borderWidth);
        }

        nvgStrokeColor(vg, colorToNVG(m_borderColor));
        nvgStrokeWidth(vg, m_borderWidth);
        nvgStroke(vg);
    }
}

NVGcolor LFNode::colorToNVG(uint32_t c) {
    return nvgRGBA((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF);
}

// ==========================================
// Event Listeners (Phase 1: Basic touch events)
// ==========================================

void LFNode::setOnTouchDown(TouchEventListener listener) {
    m_onTouchDown = listener;
}

void LFNode::setOnTouchMove(TouchEventListener listener) {
    m_onTouchMove = listener;
}

void LFNode::setOnTouchUp(TouchEventListener listener) {
    m_onTouchUp = listener;
}

void LFNode::setOnTouchCancel(TouchEventListener listener) {
    m_onTouchCancel = listener;
}

void LFNode::setOnInterceptTouchEvent(InterceptEventListener listener) {
    m_onInterceptTouchEvent = listener;
}

// ==========================================
// Gesture Recognizers (Phase 2)
// ==========================================

#include "gesture/LFGestureRecognizer.h"

void LFNode::addGestureRecognizer(std::shared_ptr<LFGestureRecognizer> recognizer) {
    if (!recognizer) return;
    m_gestureRecognizers.push_back(recognizer);
}

void LFNode::removeGestureRecognizer(std::shared_ptr<LFGestureRecognizer> recognizer) {
    if (!recognizer) return;
    auto it = std::find(m_gestureRecognizers.begin(), m_gestureRecognizers.end(), recognizer);
    if (it != m_gestureRecognizers.end()) {
        m_gestureRecognizers.erase(it);
    }
}

void LFNode::clearGestureRecognizers() {
    m_gestureRecognizers.clear();
}

void LFNode::setOnTap(TapCallback callback) {
    auto recognizer = std::make_shared<LFTapGestureRecognizer>();
    recognizer->setOnTap(callback);
    addGestureRecognizer(recognizer);
}

void LFNode::setOnDoubleTap(TapCallback callback) {
    auto recognizer = std::make_shared<LFTapGestureRecognizer>();
    recognizer->setDoubleTapEnabled(true);
    recognizer->setOnDoubleTap(callback);
    addGestureRecognizer(recognizer);
}

void LFNode::setOnLongPress(LongPressCallback callback) {
    auto recognizer = std::make_shared<LFLongPressGestureRecognizer>();
    recognizer->setOnLongPress(callback);
    addGestureRecognizer(recognizer);
}

void LFNode::setOnPan(PanUpdateCallback onUpdate,
                      PanStartCallback onStart,
                      PanEndCallback onEnd) {
    auto recognizer = std::make_shared<LFPanGestureRecognizer>();
    if (onUpdate) recognizer->setOnPanUpdate(onUpdate);
    if (onStart) recognizer->setOnPanStart(onStart);
    if (onEnd) recognizer->setOnPanEnd(onEnd);
    addGestureRecognizer(recognizer);
}
