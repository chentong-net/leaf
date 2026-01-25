//
// Created by Chen Tong on 2026/1/26.
//

#include "LFScrollView.h"
#include "LFGlobalAnimationManager.h"

LFScrollView::Ptr LFScrollView::createVertical() {
    auto instance = std::make_shared<LFScrollView>();

    instance->initLayout();
    instance->initAnimator();
    instance->initGestures();

    return instance;
}

LFScrollView::LFScrollView() {

    // 初始化物理弹簧
    // 质量1.0, 刚度150(较软), 阻尼18(适中回弹)
    m_spring.setPhysicalParameters(1.0, 150.0, 18.0);

    // 初始化物理驱动器
}

LFScrollView::~LFScrollView() {
    if (m_animator) m_animator->stop();
}

void LFScrollView::initLayout() {
    // 1. 自身作为视口 (Viewport)
    // 开启裁剪，超出部分不显示
    setMasksToBounds(true);
    // 默认撑满父容器
    matchParentWidth();
    matchParentHeight();

    // 2. 创建内容容器 (Content)
    m_content = LFBox::create();
    // 宽度撑满 ScrollView
    m_content->matchParentWidth();
    // 高度由子元素决定 (Wrap Content)
    m_content->wrapContentHeight();

    // 3. 将 Content 添加到自身 (使用基类方法)
    LFBox::addChild(m_content, LFBoxAlign::TopLeft);
}

void LFScrollView::addChild(const LFNode::Ptr& child) {
    // 拦截 addChild，全部塞给 m_content
    // 默认线性排列，所以这里假设 m_content 内部用 Flex Column
    // 如果 m_content 是 LFBox，则需要 child 自己指定位置或流式布局
    // 为了通用，建议 ScrollView 内部 Content 默认行为是 Vertical Linear 行为，
    // 但因为我们用的是 LFBox，这里依赖 Yoga 的 FlexDirectionColumn 默认值
    m_content->addChild(child, LFBoxAlign::TopLeft);
    child->setPositionType(YGPositionTypeRelative);
    child->setPosition(YGEdgeRight, NAN);
    child->setPosition(YGEdgeBottom, NAN);
    child->setTranslatePercent(0, 0);
}

void LFScrollView::addOverlayChild(const LFNode::Ptr& child, LFBoxAlign align, float offsetX, float offsetY) {
    LFBox::addChild(child, align, offsetX, offsetY);
}


void LFScrollView::initAnimator() {
    m_animator = std::make_shared<ScrollPhysicsAnimator>();
    std::weak_ptr<LFScrollView> weakSelf = std::static_pointer_cast<LFScrollView>(shared_from_this());
    m_animator->setUpdateCallback([weakSelf](float dt) {
        if (auto self = weakSelf.lock()) {
            self->updatePhysics(dt);
            return self->m_velocity == 0.0f && !self->m_inSpringMode; // 返回 true 结束动画
        }
        return true;
    });
}

void LFScrollView::initGestures() {
    // 注册拖拽手势
    auto weakSelf = std::weak_ptr<LFScrollView>(std::static_pointer_cast<LFScrollView>(shared_from_this()));

    setOnPan(
        // onUpdate
        [weakSelf](const LFPoint& delta, const LFPoint& velocity) {
            auto self = weakSelf.lock();
            if (!self) return;

            // 1. 停止之前的物理动画
            if (self->m_animator->isRunning()) {
                self->m_animator->stop();
            }
            self->m_inSpringMode = false;
            self->m_isDragging = true;
            self->showScrollbar();

            // 2. 计算位移
            float dy = delta.y;

            // 3. 阻尼效果 (Resistance)
            // 当拖拽超出边界时，施加阻力，让用户感觉拉不动
            if (self->isOutOfBounds(self->m_scrollY)) {
                dy *= 0.5f; // 移动距离减半
            }

            // 4. 应用位移
            self->m_scrollY += dy;
            self->m_content->setTranslate(0, self->m_scrollY);
            self->m_velocity = velocity.y;
        },
        // onStart
        [weakSelf](const LFPoint& delta, const LFPoint& velocity) {
            auto self = weakSelf.lock();
            if (self) {
                // 手指按下，立即停止惯性
                self->m_animator->stop();
                self->m_isDragging = true;
                self->m_velocity = 0;
            }
        },
        // onEnd
        [weakSelf](const LFPoint& delta, const LFPoint& velocity) {
            auto self = weakSelf.lock();
            if (!self) return;

            self->m_isDragging = false;
            self->m_velocity = velocity.y;

            // 手指松开，开启物理模拟 (Fling 或 Bounce)
            self->m_animator->start();
            LFGlobalAnimationManager::getInstance().addAnimator(self->m_animator);
        }
    );

    // 限制 Pan 手势只响应垂直方向
    // 这样如果子 View 有水平滑动手势（如 Banner），可以共存
    if (auto pan = std::dynamic_pointer_cast<LFPanGestureRecognizer>(getGestureRecognizers()[0])) {
        pan->setDirection(LFPanGestureRecognizer::PanDirection::Vertical);
    }
}

void LFScrollView::updatePhysics(float dt) {
    if (m_isDragging) return;

    // 状态 A: 越界回弹 (Spring Mode)
    if (isOutOfBounds(m_scrollY)) {
        if (!m_inSpringMode) {
            // 刚进入回弹状态，初始化弹簧
            m_inSpringMode = true;
            float targetY = getBoundaryY(m_scrollY);
            m_spring.setCurrentValue(m_scrollY, m_velocity);
            m_spring.setTargetValue(targetY);
        }

        // 推进弹簧模拟
        m_scrollY = (float)m_spring.advance(dt);
        m_velocity = (float)m_spring.getVelocity();

        // 检查是否静止
        if (m_spring.isAtRest()) {
            m_scrollY = (float)m_spring.getTargetValue(); // 修正到精确值
            m_velocity = 0;
            m_inSpringMode = false;
            m_animator->stop(); // 结束动画
            hideScrollbar();
        }
    }
    // 状态 B: 惯性滑动 (Fling Mode)
    else {
        m_inSpringMode = false;

        // 摩擦力衰减公式: v = v * pow(friction, dt * 60)
        // 简单起见使用线性近似
        float frictionFactor = std::pow(FRICTION, dt * 60.0f); // 60fps基准
        m_velocity *= frictionFactor;

        // 位移更新: s = s + v * dt
        m_scrollY += m_velocity * dt;

        // 停止条件
        if (std::abs(m_velocity) < MIN_VELOCITY) {
            m_velocity = 0;
            m_animator->stop();
            hideScrollbar();
        }

        // 特殊情况：Fling 过程中冲出了边界
        // 下一帧会自动进入 Spring Mode
    }

    // 应用最终位置
    m_content->setTranslate(0, m_scrollY);
}

float LFScrollView::getBoundaryY(float targetY) const {
    float viewH = getLayoutHeight();
    float contentH = m_content->getLayoutHeight();

    // 1. 内容比视口小：不允许滚动，强制吸附顶部 (0)
    if (contentH <= viewH) {
        return 0.0f;
    }

    // 2. 顶部边界检查
    if (targetY > 0) {
        return 0.0f;
    }

    // 3. 底部边界检查
    // scrollY 是负数，最小值为 -(contentH - viewH)
    float minScroll = -(contentH - viewH);
    if (targetY < minScroll) {
        return minScroll;
    }

    return targetY;
}

bool LFScrollView::isOutOfBounds(float y) const {
    return y != getBoundaryY(y);
}

void LFScrollView::scrollTo(float y, bool animate) {
    // 还没做主动 scrollTo 的动画，先直接跳过去
    // TODO: 可以复用 m_spring 做平滑滚动
    m_scrollY = getBoundaryY(y);
    m_content->setTranslate(0, m_scrollY);
}

// ==========================================
// 滚动条绘制
// ==========================================

void LFScrollView::setScrollBarEnabled(bool enabled) {
    if (m_scrollBarEnabled == enabled) return;
    m_scrollBarEnabled = enabled;

    // 如果被关闭，立即隐藏当前可能正在显示的滚动条
    if (!enabled) {
        m_scrollbarOpacity = 0.0f;
        if (m_barFadeAnimator && m_barFadeAnimator->isRunning()) {
            m_barFadeAnimator->stop();
        }
        markDirty(); // 触发重绘以清除残影
    }
}

void LFScrollView::onDrawOverlay(NVGcontext* vg) {
    if (!m_scrollBarEnabled) return;
    if (m_scrollbarOpacity <= 0) return;

    float viewH = getLayoutHeight();
    float viewW = getLayoutWidth();
    float contentH = m_content->getLayoutHeight();

    if (contentH <= viewH) return; // 内容不够长，不显示滚动条

    // 滚动条参数
    float barWidth = 4.0f;
    float paddingRight = 2.0f;

    // 计算滚动条长度 (比例)
    float ratio = viewH / contentH;
    float barHeight = viewH * ratio;

    // 限制最小长度，防止太短看不到
    barHeight = std::max(barHeight, 20.0f);

    // 计算滚动条位置
    // 映射关系: scrollY (0 ~ minScroll) -> barY (0 ~ viewH - barHeight)
    float maxScroll = contentH - viewH; // 正数
    float scrollProgress = -m_scrollY / maxScroll; // 0.0 ~ 1.0 (可能 <0 或 >1 如果越界)

    float trackSpace = viewH - barHeight;
    float barY = scrollProgress * trackSpace;

    // 视觉修正：弹簧越界时，滚动条会缩短，这里简单处理让它跟随
    // 如果需要更高级的 iOS 效果(越界缩短)，可以进一步计算

    nvgSave(vg);
    nvgGlobalAlpha(vg, m_scrollbarOpacity);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, viewW - barWidth - paddingRight, barY, barWidth, barHeight, barWidth / 2);
    nvgFillColor(vg, nvgRGBA(100, 100, 100, 180)); // 半透明灰
    nvgFill(vg);

    nvgRestore(vg);
}

void LFScrollView::showScrollbar() {
    if (!m_scrollBarEnabled) return;
    m_scrollbarOpacity = 1.0f;
    if (m_barFadeAnimator) {
        m_barFadeAnimator->stop();
    }
    markDirty(); // 触发重绘以显示
}

void LFScrollView::hideScrollbar() {
    // 淡出动画
    if (!m_scrollBarEnabled) return;
    if (m_scrollbarOpacity <= 0) return;

    m_barFadeAnimator = LFValueAnimator<float>::of(m_scrollbarOpacity, 0.0f);
    m_barFadeAnimator->setDuration(0.3f);

    std::weak_ptr<LFScrollView> weakSelf = std::static_pointer_cast<LFScrollView>(shared_from_this());
    m_barFadeAnimator->addUpdateListener([weakSelf](const float& val) {
        if (auto self = weakSelf.lock()) {
            self->m_scrollbarOpacity = val;
            self->markDirty(); // 重绘
        }
    });

    m_barFadeAnimator->start();
    LFGlobalAnimationManager::getInstance().addAnimator(m_barFadeAnimator);
}
