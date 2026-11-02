//
// Created by Chen Tong on 2026/1/17.
//

#include "view/base/LFNode.h"
#include "event/LFEventDispatcher.h"
#include "layout/LFLayoutFactory.h"

#include <utility>

// 简化 setter 编写，只有值变了才 markDirty
#define SET_STYLE_VAL(prop, value) \
    if (prop == value) return; \
    prop = value; \
    markDirty();

namespace {

constexpr float kPi = 3.14159265358979323846f;

LFLayoutValue pointValue(float value) {
    return std::isfinite(value)
               ? LFLayoutValue{value, LFLayoutUnit::Point}
               : LFLayoutValue{};
}

LFLayoutValue percentValue(float value) {
    return std::isfinite(value)
               ? LFLayoutValue{value, LFLayoutUnit::Percent}
               : LFLayoutValue{};
}

float resolveOwnerAxis(const LFLayoutValue& axis, float ownerSize) {
    if (axis.unit == LFLayoutUnit::Point) {
        return axis.value;
    }
    if (axis.unit == LFLayoutUnit::Percent && std::isfinite(ownerSize)) {
        return ownerSize * axis.value / 100.0f;
    }
    return std::numeric_limits<float>::quiet_NaN();
}

bool pointInRoundedRect(float x, float y, float width, float height, float radius) {
    radius = std::min(radius, std::min(width, height) * 0.5f);
    const bool inLeft = x < radius;
    const bool inRight = x > width - radius;
    const bool inTop = y < radius;
    const bool inBottom = y > height - radius;

    if (inLeft && inTop) {
        const float dx = x - radius;
        const float dy = y - radius;
        return dx * dx + dy * dy <= radius * radius;
    }
    if (inRight && inTop) {
        const float dx = x - (width - radius);
        const float dy = y - radius;
        return dx * dx + dy * dy <= radius * radius;
    }
    if (inLeft && inBottom) {
        const float dx = x - radius;
        const float dy = y - (height - radius);
        return dx * dx + dy * dy <= radius * radius;
    }
    if (inRight && inBottom) {
        const float dx = x - (width - radius);
        const float dy = y - (height - radius);
        return dx * dx + dy * dy <= radius * radius;
    }
    return true;
}

}

LFNode::LFNode()
    : m_layout(LFLayoutFactory::create()) {
    m_layout->setStyle(m_layoutStyle);
}

LFNode::~LFNode() = default;

// ==========================================
// 树管理实现
// ==========================================
void LFNode::addChild(const Ptr& child) {
    if (!child || child.get() == this) {
        return;
    }
    for (const LFNode* ancestor = this; ancestor; ancestor = ancestor->m_parent) {
        if (ancestor == child.get()) {
            return;
        }
    }
    if (child->getParent()) {
        child->removeFromParent();
    }
    child->m_parent = this;
    m_children.push_back(child);
    m_layout->insertChild(*child->m_layout, m_children.size() - 1);
    markDirty();
}

void LFNode::insertChild(const Ptr& child, uint32_t index) {
    if (!child || child.get() == this) {
        return;
    }
    for (const LFNode* ancestor = this; ancestor; ancestor = ancestor->m_parent) {
        if (ancestor == child.get()) {
            return;
        }
    }
    if (child->getParent()) {
        child->removeFromParent();
    }
    if (static_cast<size_t>(index) > m_children.size()) {
        index = static_cast<uint32_t>(m_children.size());
    }

    child->m_parent = this;
    m_children.insert(m_children.begin() + index, child);
    m_layout->insertChild(*child->m_layout, index);
    markDirty();
}

void LFNode::removeChild(const Ptr& child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        m_layout->removeChild(*child->m_layout);
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
// 布局属性实现
// ==========================================
void LFNode::setWidth(float width) {
    const LFLayoutValue value = !std::isfinite(width)
                                    ? LFLayoutValue{}
                                    : width < 0.0f
                                    ? LFLayoutValue{0.0f, LFLayoutUnit::Auto}
                                    : LFLayoutValue{width, LFLayoutUnit::Point};
    if (m_layoutStyle.width == value) return;
    m_layoutStyle.width = value;
    applyLayoutStyle();
}
void LFNode::setHeight(float height) {
    const LFLayoutValue value = !std::isfinite(height)
                                    ? LFLayoutValue{}
                                    : height < 0.0f
                                    ? LFLayoutValue{0.0f, LFLayoutUnit::Auto}
                                    : LFLayoutValue{height, LFLayoutUnit::Point};
    if (m_layoutStyle.height == value) return;
    m_layoutStyle.height = value;
    applyLayoutStyle();
}
void LFNode::setWidthPercent(float percent) {
    const LFLayoutValue value = percentValue(percent);
    if (m_layoutStyle.width == value) return;
    m_layoutStyle.width = value;
    applyLayoutStyle();
}

void LFNode::setHeightPercent(float percent) {
    const LFLayoutValue value = percentValue(percent);
    if (m_layoutStyle.height == value) return;
    m_layoutStyle.height = value;
    applyLayoutStyle();
}
void LFNode::setMinWidth(float minWidth) {
    const LFLayoutValue value = pointValue(minWidth);
    if (m_layoutStyle.minWidth == value) return;
    m_layoutStyle.minWidth = value;
    applyLayoutStyle();
}
void LFNode::setMaxWidth(float maxWidth) {
    const LFLayoutValue value = pointValue(maxWidth);
    if (m_layoutStyle.maxWidth == value) return;
    m_layoutStyle.maxWidth = value;
    applyLayoutStyle();
}
void LFNode::setMinHeight(float minHeight) {
    const LFLayoutValue value = pointValue(minHeight);
    if (m_layoutStyle.minHeight == value) return;
    m_layoutStyle.minHeight = value;
    applyLayoutStyle();
}
void LFNode::setMaxHeight(float maxHeight) {
    const LFLayoutValue value = pointValue(maxHeight);
    if (m_layoutStyle.maxHeight == value) return;
    m_layoutStyle.maxHeight = value;
    applyLayoutStyle();
}
void LFNode::setAspectRatio(float aspectRatio) {
    const float value = std::isfinite(aspectRatio)
                            ? aspectRatio
                            : std::numeric_limits<float>::quiet_NaN();
    if ((std::isnan(m_layoutStyle.aspectRatio) && std::isnan(value)) ||
        m_layoutStyle.aspectRatio == value) return;
    m_layoutStyle.aspectRatio = value;
    applyLayoutStyle();
}

void LFNode::matchParentWidth() {
    setWidthPercent(100.0f);
}

void LFNode::matchParentHeight() {
    setHeightPercent(100.0f);
}

void LFNode::wrapContentWidth() {
    const LFLayoutValue value{0.0f, LFLayoutUnit::Auto};
    if (m_layoutStyle.width == value) return;
    m_layoutStyle.width = value;
    applyLayoutStyle();
}

void LFNode::wrapContentHeight() {
    const LFLayoutValue value{0.0f, LFLayoutUnit::Auto};
    if (m_layoutStyle.height == value) return;
    m_layoutStyle.height = value;
    applyLayoutStyle();
}

void LFNode::setFlexDirection(LFFlexDirection direction) {
    if (m_layoutStyle.flexDirection == direction) return;
    m_layoutStyle.flexDirection = direction;
    applyLayoutStyle();
}
void LFNode::setJustifyContent(LFJustify justify) {
    if (m_layoutStyle.justifyContent == justify) return;
    m_layoutStyle.justifyContent = justify;
    applyLayoutStyle();
}
void LFNode::setAlignItems(LFAlign align) {
    if (m_layoutStyle.alignItems == align) return;
    m_layoutStyle.alignItems = align;
    applyLayoutStyle();
}
void LFNode::setAlignSelf(LFAlign align) {
    if (m_layoutStyle.alignSelf == align) return;
    m_layoutStyle.alignSelf = align;
    applyLayoutStyle();
}
void LFNode::setAlignContent(LFAlign align) {
    if (m_layoutStyle.alignContent == align) return;
    m_layoutStyle.alignContent = align;
    applyLayoutStyle();
}
void LFNode::setFlexWrap(LFWrap wrap) {
    if (m_layoutStyle.flexWrap == wrap) return;
    m_layoutStyle.flexWrap = wrap;
    applyLayoutStyle();
}
void LFNode::setFlexGrow(float grow) {
    const float value = std::isfinite(grow) ? grow : 0.0f;
    if (m_layoutStyle.flexGrow == value) return;
    m_layoutStyle.flexGrow = value;
    applyLayoutStyle();
}
void LFNode::setFlexShrink(float shrink) {
    const float value = std::isfinite(shrink) ? shrink : 0.0f;
    if (m_layoutStyle.flexShrink == value) return;
    m_layoutStyle.flexShrink = value;
    applyLayoutStyle();
}
void LFNode::setFlexBasis(float basis) {
    const LFLayoutValue value = std::isfinite(basis)
                                    ? LFLayoutValue{basis, LFLayoutUnit::Point}
                                    : LFLayoutValue{0.0f, LFLayoutUnit::Auto};
    if (m_layoutStyle.flexBasis == value) return;
    m_layoutStyle.flexBasis = value;
    applyLayoutStyle();
}

void LFNode::setPadding(LFLayoutEdge edge, float padding) {
    const LFLayoutValue value = pointValue(padding);
    auto& target = m_layoutStyle.padding[static_cast<size_t>(edge)];
    if (target == value) return;
    target = value;
    applyLayoutStyle();
}

void LFNode::setMargin(LFLayoutEdge edge, float margin) {
    const LFLayoutValue value = pointValue(margin);
    auto& target = m_layoutStyle.margin[static_cast<size_t>(edge)];
    if (target == value) return;
    target = value;
    applyLayoutStyle();
}

void LFNode::setGap(LFGutter gutter, float gap) {
    const float value = std::isfinite(gap)
                            ? gap
                            : std::numeric_limits<float>::quiet_NaN();
    auto& target = m_layoutStyle.gap[static_cast<size_t>(gutter)];
    if ((std::isnan(target) && std::isnan(value)) || target == value) return;
    target = value;
    applyLayoutStyle();
}

void LFNode::setPositionType(LFPositionType type) {
    if (m_layoutStyle.positionType == type) return;
    m_layoutStyle.positionType = type;
    applyLayoutStyle();
}

void LFNode::setPosition(LFLayoutEdge edge, float value) {
    const LFLayoutValue layoutValue = pointValue(value);
    auto& target = m_layoutStyle.position[static_cast<size_t>(edge)];
    if (target == layoutValue) return;
    target = layoutValue;
    applyLayoutStyle();
}

void LFNode::setPositionPercent(LFLayoutEdge edge, float percent) {
    const LFLayoutValue value = percentValue(percent);
    auto& target = m_layoutStyle.position[static_cast<size_t>(edge)];
    if (target == value) return;
    target = value;
    applyLayoutStyle();
}

void LFNode::setDisplay(LFDisplay display) {
    if (m_layoutStyle.display == display) return;
    m_layoutStyle.display = display;
    applyLayoutStyle();
}

void LFNode::setDirection(LFDirection direction) {
    if (m_layoutStyle.direction == direction) return;
    m_layoutStyle.direction = direction;
    applyLayoutStyle();
}

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
    m_layoutStyle.border[static_cast<size_t>(LFLayoutEdge::All)] = {
        m_borderWidth,
        LFLayoutUnit::Point
    };
    applyLayoutStyle();
}

void LFNode::setLayoutBorderWidth(LFLayoutEdge edge, float width) {
    const LFLayoutValue value{
        std::isfinite(width) ? std::max(0.0f, width) : 0.0f,
        LFLayoutUnit::Point
    };
    auto& target = m_layoutStyle.border[static_cast<size_t>(edge)];
    if (target == value) return;
    target = value;
    applyLayoutStyle();
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
    // 布局或视觉变化需要让祖先节点参与当前帧的重绘。
    if (m_parent) m_parent->markDirty();
}

void LFNode::invalidateLayout() {
    if (m_layout) {
        m_layout->markDirty();
    }
    markDirty();
}

void LFNode::applyLayoutStyle() {
    if (m_layout) {
        m_layout->setStyle(m_layoutStyle);
    }
    markDirty();
}

void LFNode::setLayoutMeasureFunction(LFMeasureFunction function) {
    if (m_layout) {
        m_layout->setMeasureFunction(std::move(function));
    }
}

void LFNode::setLayoutBaselineFunction(LFBaselineFunction function) {
    if (m_layout) {
        m_layout->setBaselineFunction(std::move(function));
    }
}

void LFNode::calculateLayout(float ownerWidth, float ownerHeight) {
    prepareLayoutTree(ownerWidth, ownerHeight);

    m_layout->calculate({ownerWidth, ownerHeight});
    updateLayoutFrameTree();

    finalizeLayoutTree();
    m_isDirty = false;
}

void LFNode::prepareLayoutTree(float ownerWidth, float ownerHeight) {
    onBeforeCalculateLayout(ownerWidth, ownerHeight);

    float childOwnerWidth = ownerWidth;
    float childOwnerHeight = ownerHeight;

    const float resolvedSelfWidth = resolveOwnerAxis(m_layoutStyle.width, ownerWidth);
    const float resolvedSelfHeight = resolveOwnerAxis(m_layoutStyle.height, ownerHeight);

    if (std::isfinite(resolvedSelfWidth)) {
        childOwnerWidth = resolvedSelfWidth;
    }
    if (std::isfinite(resolvedSelfHeight)) {
        childOwnerHeight = resolvedSelfHeight;
    }

    for (auto& child : m_children) {
        child->prepareLayoutTree(childOwnerWidth, childOwnerHeight);
    }
}

void LFNode::finalizeLayoutTree() {
    onAfterCalculateLayout();

    for (auto& child : m_children) {
        child->finalizeLayoutTree();
    }
}

void LFNode::updateLayoutFrameTree() {
    m_layout->updateFrame();

    for (auto& child : m_children) {
        child->updateLayoutFrameTree();
    }
}

/**
 * 核心渲染流程 (Template Method)
 */
void LFNode::render(LFPaint& paint) {
    // 0. 基础过滤：如果隐藏或透明度为0，直接跳过
    // Display::None 的节点宽高为 0，也会在这里被过滤
    if (!m_visible || m_opacity <= 0.0f) return;

    float w = getLayoutWidth();
    float h = getLayoutHeight();
    if (w <= 0 || h <= 0) return;

    // 1. 保存当前绘制状态
    paint.pushState();

    // 2. 应用布局坐标 (Layout X/Y)
    float x = getLayoutX();
    float y = getLayoutY();
    paint.translate(x, y);

    // 3. 应用视觉变换 (Transform: Translate -> Rotate -> Scale)
    // 变换原点默认中心
    float tx = m_transform.translateX;
    float ty = m_transform.translateY;
    tx += w * (m_transform.translatePercentX / 100.0f);
    ty += h * (m_transform.translatePercentY / 100.0f);
    if (tx != 0 || ty != 0) paint.translate(tx, ty);

    if (m_transform.rotate != 0 || m_transform.scaleX != 1.0f || m_transform.scaleY != 1.0f) {
        float cx = w * 0.5f;
        float cy = h * 0.5f;
        paint.translate(cx, cy);
        if (m_transform.rotate != 0) paint.rotate(m_transform.rotate);
        if (m_transform.scaleX != 1.0f || m_transform.scaleY != 1.0f) {
            paint.scale(m_transform.scaleX, m_transform.scaleY);
        }
        paint.translate(-cx, -cy);
    }

    // 4. 应用透明度
    if (m_opacity < 1.0f) {
        paint.setOpacity(m_opacity);
    }

    // 5. 绘制阴影 (在背景之前)
    drawShadow(paint, w, h);

    // 6. 绘制背景
    drawBackground(paint, w, h);

    // 8. 裁剪 (Overflow: Hidden)
    if (m_masksToBounds) {
        paint.clipRect({0.0f, 0.0f, w, h});
    }

    // 7. 绘制内容 (子类如 Text/Image)
    onDrawContent(paint);

    // 9. 递归绘制子节点
    for (auto& child : m_children) {
        child->render(paint);
    }

    // 10. 绘制边框 (在子节点之上，防止被内容遮挡)
    drawBorder(paint, w, h);

    onDrawOverlay(paint);

    // 11. 恢复状态
    paint.popState();
}

// ==========================================
// 内部绘制实现
// ==========================================
void LFNode::drawShadow(LFPaint& paint, float w, float h) {
    if ((m_shadow.color >> 24) & 0xFF) {
        LFShadowDraw shadow;
        shadow.rect = {0.0f, 0.0f, w, h};
        shadow.radius = m_borderRadius;
        shadow.offsetX = m_shadow.offsetX;
        shadow.offsetY = m_shadow.offsetY;
        shadow.blur = m_shadow.blurRadius;
        shadow.spread = m_shadow.spread;
        shadow.color = m_shadow.color;
        paint.drawShadow(shadow);
    }
}

void LFNode::drawBackground(LFPaint& paint, float w, float h) {
    if ((m_backgroundColor >> 24) & 0xFF) {
        if (m_borderRadius > 0) {
            paint.drawRoundedRect({0.0f, 0.0f, w, h}, m_borderRadius, m_backgroundColor);
        } else {
            paint.drawRect({0.0f, 0.0f, w, h}, m_backgroundColor);
        }
    }
}

void LFNode::drawBorder(LFPaint& paint, float w, float h) {
    // 统一边框绘制
    // 只有当设置了边框宽且颜色不透明时才绘制
    if (m_borderWidth > 0.0f && ((m_borderColor >> 24) & 0xFF)) {
        // NanoVG 的 Stroke 是居中的。为了让边框完全在节点内(Border-Box模型)，
        // 我们需要向内缩半个边框宽度。
        float half = m_borderWidth * 0.5f;

        if (m_borderRadius > 0) {
            // 圆角也要相应收缩，防止内部圆角异常
            float innerRadius = std::max(0.0f, m_borderRadius - half);
            paint.strokeRoundedRect(
                {half, half, w - m_borderWidth, h - m_borderWidth},
                innerRadius,
                m_borderColor,
                m_borderWidth
            );
        } else {
            paint.strokeRect(
                {half, half, w - m_borderWidth, h - m_borderWidth},
                m_borderColor,
                m_borderWidth
            );
        }
    }
}

bool LFNode::convertWindowPointToLocal(float windowX, float windowY,
                                       float& localX, float& localY) const {
    std::vector<const LFNode*> chain;
    for (const LFNode* node = this; node; node = node->m_parent) {
        chain.push_back(node);
    }

    float x = windowX;
    float y = windowY;
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        const LFNode* node = *it;
        x -= node->getLayoutX();
        y -= node->getLayoutY();

        const LFTransform& transform = node->m_transform;
        const float width = node->getLayoutWidth();
        const float height = node->getLayoutHeight();
        x -= transform.translateX + width * transform.translatePercentX / 100.0f;
        y -= transform.translateY + height * transform.translatePercentY / 100.0f;

        if (transform.rotate == 0.0f &&
            transform.scaleX == 1.0f &&
            transform.scaleY == 1.0f) {
            continue;
        }
        if (std::abs(transform.scaleX) < 0.001f ||
            std::abs(transform.scaleY) < 0.001f) {
            return false;
        }

        const float centerX = width * 0.5f;
        const float centerY = height * 0.5f;
        x -= centerX;
        y -= centerY;

        if (transform.rotate != 0.0f) {
            const float radians = -transform.rotate * kPi / 180.0f;
            const float cosine = std::cos(radians);
            const float sine = std::sin(radians);
            const float rotatedX = x * cosine - y * sine;
            const float rotatedY = x * sine + y * cosine;
            x = rotatedX;
            y = rotatedY;
        }
        x /= transform.scaleX;
        y /= transform.scaleY;
        x += centerX;
        y += centerY;
    }

    localX = x;
    localY = y;
    return true;
}

void LFNode::convertLocalPointToWindow(float& localX, float& localY) const {
    float x = localX;
    float y = localY;
    for (const LFNode* node = this; node; node = node->m_parent) {
        const LFTransform& transform = node->m_transform;
        const float width = node->getLayoutWidth();
        const float height = node->getLayoutHeight();
        const float centerX = width * 0.5f;
        const float centerY = height * 0.5f;

        x -= centerX;
        y -= centerY;
        x *= transform.scaleX;
        y *= transform.scaleY;
        if (transform.rotate != 0.0f) {
            const float radians = transform.rotate * kPi / 180.0f;
            const float cosine = std::cos(radians);
            const float sine = std::sin(radians);
            const float rotatedX = x * cosine - y * sine;
            const float rotatedY = x * sine + y * cosine;
            x = rotatedX;
            y = rotatedY;
        }
        x += centerX;
        y += centerY;

        x += transform.translateX + width * transform.translatePercentX / 100.0f;
        y += transform.translateY + height * transform.translatePercentY / 100.0f;
        x += node->getLayoutX();
        y += node->getLayoutY();
    }

    localX = x;
    localY = y;
}

bool LFNode::containsWindowPoint(float windowX, float windowY) const {
    if (!m_visible || m_opacity <= 0.0f) {
        return false;
    }

    float localX = 0.0f;
    float localY = 0.0f;
    if (!convertWindowPointToLocal(windowX, windowY, localX, localY)) {
        return false;
    }

    const float width = getLayoutWidth();
    const float height = getLayoutHeight();
    if (localX < 0.0f || localX > width || localY < 0.0f || localY > height) {
        return false;
    }

    const float radius = m_borderRadius;
    if (radius <= 0.0f) {
        return true;
    }

    return pointInRoundedRect(localX, localY, width, height, radius);
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

void LFNode::setOnScroll(ScrollEventListener listener) {
    m_onScroll = listener;
}

void LFNode::setOnInterceptTouchEvent(InterceptEventListener listener) {
    m_onInterceptTouchEvent = listener;
}

void LFNode::setOnKeyDown(KeyEventListener listener) {
    m_onKeyDown = listener;
}

void LFNode::setOnKeyUp(KeyEventListener listener) {
    m_onKeyUp = listener;
}

void LFNode::setOnCharInput(KeyEventListener listener) {
    m_onCharInput = listener;
}

void LFNode::setOnFocusChange(FocusChangeListener listener) {
    m_onFocusChange = listener;
}

void LFNode::setOnHoverChange(HoverEventListener listener) {
    m_onHoverChange = listener;
}

void LFNode::requestFocus() {
    if (!m_focusable) return;
    LFEventDispatcher::getInstance().setFocusedNode(shared_from_this());
}

void LFNode::clearFocus() {
    LFEventDispatcher::getInstance().clearFocus(shared_from_this());
}

void LFNode::setFocusState(bool focused) {
    if (m_hasFocus == focused) return;
    m_hasFocus = focused;
    if (m_onFocusChange) {
        m_onFocusChange(focused);
    }
    onFocusChanged(focused);
    markDirty();
}

void LFNode::setHoverState(bool hovered) {
    if (m_isHovered == hovered) return;
    m_isHovered = hovered;
    if (m_onHoverChange) {
        m_onHoverChange(hovered);
    }
    onHoverChanged(hovered);
    markDirty();
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

void LFNode::setOnPinch(PinchCallback onUpdate,
                        PinchCallback onStart,
                        PinchCallback onEnd) {
    auto recognizer = std::make_shared<LFPinchGestureRecognizer>();
    if (onUpdate) recognizer->setOnPinchUpdate(onUpdate);
    if (onStart) recognizer->setOnPinchStart(onStart);
    if (onEnd) recognizer->setOnPinchEnd(onEnd);
    addGestureRecognizer(recognizer);
}

void LFNode::setOnRotate(RotateCallback onUpdate,
                         RotateCallback onStart,
                         RotateCallback onEnd) {
    auto recognizer = std::make_shared<LFRotateGestureRecognizer>();
    if (onUpdate) recognizer->setOnRotateUpdate(onUpdate);
    if (onStart) recognizer->setOnRotateStart(onStart);
    if (onEnd) recognizer->setOnRotateEnd(onEnd);
    addGestureRecognizer(recognizer);
}

void LFNode::setOnSwipe(SwipeCallback callback, int allowedDirections) {
    auto recognizer = std::make_shared<LFSwipeGestureRecognizer>();
    recognizer->setOnSwipe([callback](LFSwipeGestureRecognizer::SwipeDirection dir, const LFPoint& vel) {
        callback((int)dir, vel);
    });
    recognizer->setAllowedDirections(allowedDirections);
    addGestureRecognizer(recognizer);
}
