//
// Created by Chen Tong on 2026/1/17.
//

#include "LFNode.h"
#include <algorithm>

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
    } else {
        SET_YOGA_VAL(YGNodeStyleSetHeight, height)
    }
}
void LFNode::setMinWidth(float minWidth) { SET_YOGA_VAL(YGNodeStyleSetMinWidth, minWidth); }
void LFNode::setMaxWidth(float maxWidth) { SET_YOGA_VAL(YGNodeStyleSetMaxWidth, maxWidth); }
void LFNode::setMinHeight(float minHeight) { SET_YOGA_VAL(YGNodeStyleSetMinHeight, minHeight); }
void LFNode::setMaxHeight(float maxHeight) { SET_YOGA_VAL(YGNodeStyleSetMaxHeight, maxHeight); }
void LFNode::setAspectRatio(float aspectRatio) { SET_YOGA_VAL(YGNodeStyleSetAspectRatio, aspectRatio); }

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
void LFNode::setBorderWidth(YGEdge edge, float width) { SET_YOGA_VAL(YGNodeStyleSetBorder, edge, width); }

void LFNode::setPositionType(YGPositionType type) { SET_YOGA_VAL(YGNodeStyleSetPositionType, type); }
void LFNode::setPosition(YGEdge edge, float value) { SET_YOGA_VAL(YGNodeStyleSetPosition, edge, value); }
void LFNode::setDisplay(YGDisplay display) { SET_YOGA_VAL(YGNodeStyleSetDisplay, display); }
void LFNode::setDirection(YGDirection direction) { SET_YOGA_VAL(YGNodeStyleSetDirection, direction); }

// ==========================================
// 样式属性实现
// ==========================================
void LFNode::setBackgroundColor(uint32_t color) { SET_STYLE_VAL(m_backgroundColor, color); }
void LFNode::setOpacity(float opacity) { SET_STYLE_VAL(m_opacity, std::clamp(opacity, 0.0f, 1.0f)); }
void LFNode::setVisible(bool visible) { SET_STYLE_VAL(m_visible, visible); }
void LFNode::setBorderColor(uint32_t color) { SET_STYLE_VAL(m_borderColor, color); }
void LFNode::setBorderRadius(float radius) { SET_STYLE_VAL(m_borderRadius, radius); }
void LFNode::setMasksToBounds(bool masks) { SET_STYLE_VAL(m_masksToBounds, masks); }

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
    // 根节点触发计算
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
    if (m_transform.translateX != 0 || m_transform.translateY != 0)
        nvgTranslate(vg, m_transform.translateX, m_transform.translateY);

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

    // 7. 绘制内容 (子类如 Text/Image)
    onDrawContent(vg);

    // 8. 裁剪 (Overflow: Hidden)
    if (m_masksToBounds) {
        // IntersectScissor 是基于当前 Transform 的
        nvgIntersectScissor(vg, 0, 0, w, h);
    }

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
    // Yoga 的 Border 属性只是占位，视觉绘制需要在这里单独处理
    // 我们可以简单读取 Layout 里的 border 宽度，或者使用自定义的样式属性
    float borderW = YGNodeStyleGetBorder(m_ygNode, YGEdgeAll);
    if(std::isnan(borderW) || borderW <= 0) {
        // 如果没有单独设置，尝试读取 fallback
        // 这里为了演示简单，假设我们用了一个 m_borderWidth 成员变量
        // 实际可以将 Yoga Border 和视觉 Border 分离或统一
        // 暂时使用 style proxy 里的逻辑
    }

    // 如果你在头文件里定义了 setBorderWidth, 最好将其存下来用于绘制
    // 下面假设使用样式属性 m_borderColor 和 Yoga 布局属性
    float yogaBorder = YGNodeLayoutGetBorder(m_ygNode, YGEdgeLeft); // 获取计算后的边框宽

    if (yogaBorder > 0 && ((m_borderColor >> 24) & 0xFF)) {
        float half = yogaBorder * 0.5f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, half, half, w - yogaBorder, h - yogaBorder, std::max(0.0f, m_borderRadius - half));
        nvgStrokeColor(vg, colorToNVG(m_borderColor));
        nvgStrokeWidth(vg, yogaBorder);
        nvgStroke(vg);
    }
}

NVGcolor LFNode::colorToNVG(uint32_t c) {
    return nvgRGBA((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF);
}