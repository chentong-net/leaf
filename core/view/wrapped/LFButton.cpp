//
// Created by Chen Tong on 2026/1/24.
//

//
// Created by Leaf Engine Team.
// UI Component - Button Implementation
//

#include "LFButton.h"
#include "LFEngine.h"

LFButton::Ptr LFButton::create(const std::string& text, ClickCallback onClick) {
    auto btn = std::make_shared<LFButton>();
    if (!text.empty()) {
        btn->setText(text);
    }
    if (onClick) {
        btn->setOnClick(onClick);
    }
    return btn;
}

LFButton::LFButton() {
    // 默认样式
    setClickEffect(LFClickEffect::Scale);

    // 默认布局属性：居中
    setJustifyContent(YGJustifyCenter);
    setAlignItems(YGAlignCenter);

    // 开启触摸
    setTouchEnabled(true);

    // 初始化状态颜色 (默认透明)
    m_stateColors[LFButtonState::Normal] = 0x00000000;
    m_stateColors[LFButtonState::Pressed] = 0x00000000;
    m_stateColors[LFButtonState::Disabled] = 0x00000000;

    initEvents();
}

LFButton::~LFButton() {
    if (m_scaleAnimator) {
        m_scaleAnimator->stop();
    }
    if (m_colorAnimator) {
        m_colorAnimator->stop();
    }
}

void LFButton::initEvents() {
    // 1. 按下 (TouchDown)
    setOnTouchDown([this](const LFTouchEvent& e) {
        if (!m_isEnabled) return;

        m_isTouching = true;
        m_isInside = true;

        // 进入按下状态
        updateState(LFButtonState::Pressed);
    });

    // 2. 移动 (TouchMove) - 实现“移出取消、移回恢复”
    setOnTouchMove([this](const LFTouchEvent& e) {
        if (!m_isEnabled || !m_isTouching) return;

        const LFTouchPoint* touch = e.getPrimaryTouch();
        if (!touch) return;

        // 检查手指是否还在按钮范围内 (增加 10px 的容错缓冲区)
        float tolerance = 10.0f;
        // 将屏幕坐标转换为本地坐标比较麻烦，这里我们简化逻辑：
        // 假设 HitTest 已经保证了 Down 在这里。
        // 我们利用 HitTestEngine 的逻辑或者简单的 AABB 检测
        // 由于 event 里的坐标是屏幕坐标，我们需要这一帧的节点全局位置。
        // 但 LFNode 目前没有缓存全局位置。
        // **完美方案**：使用 LFHitTestEngine 的逆矩阵逻辑转换。
        // 这里为了性能，我们假设 touchMove 持续发生。

        // 简化实现：利用 EventDispatcher 传来的 target。
        // 实际上，现代 UI 框架通常捕获了 Touch，所以 Move 事件总是发给这个 Button，
        // 我们需要判断坐标是否出界。

        // 我们需要一个 helper: isScreenPointInside(x, y)
        // 暂时 hack: 假设我们无法简单获取 Global Rect。
        // 我们可以只判断“逻辑上的移出”

        // [更正] 实际上 LFHitTest 提供了逆变换。但为了代码独立性，
        // 我们暂时认为：只要 Move 事件还在分发给我们，且 target 是我们，
        // 我们就需要自己算。
        // 由于缺乏 GlobalToLocal API，我们暂时略过精细的 Bounds Check，
        // 或者后续在 LFNode 补充 mapToLocal(point).

        // 这里先假定一直 Inside，直到实现 mapToLocal。
        // TODO: Implement mapToLocal for perfect "slide out to cancel"

        // 逻辑：如果支持 TouchCapture，手指滑出屏幕外，Button 依然收到事件。
    });

    // 3. 抬起 (TouchUp)
    setOnTouchUp([this](const LFTouchEvent& e) {
        if (!m_isEnabled || !m_isTouching) return;

        m_isTouching = false;

        const LFTouchPoint* touch = e.getPrimaryTouch();
        bool validClick = false;

        // 如果手指在范围内抬起 (这里需要严格的 Bounds Check)
        // 暂时假设只要收到了 Up 且之前没 Cancel 就算点击
        // (完善的系统需要配合 mapToLocal)
        if (m_isInside) {
            validClick = true;
        }

        // 恢复状态
        updateState(LFButtonState::Normal);

        // 触发回调
        if (validClick && m_onClick) {
            m_onClick(this);
        }
    });

    // 4. 取消 (TouchCancel)
    setOnTouchCancel([this](const LFTouchEvent& e) {
        m_isTouching = false;
        m_isInside = false;
        updateState(LFButtonState::Normal);
    });
}

// ==========================================
// 属性设置
// ==========================================

void LFButton::setEnabled(bool enabled) {
    if (m_isEnabled == enabled) return;
    m_isEnabled = enabled;
    updateState(m_isEnabled ? LFButtonState::Normal : LFButtonState::Disabled, false);
}

void LFButton::setOnClick(ClickCallback callback) {
    m_onClick = callback;
}

void LFButton::setBackgroundColor(LFButtonState state, uint32_t color) {
    m_stateColors[state] = color;
    // 如果设置的是当前状态的颜色，立即刷新
    if (state == m_state) {
        if (m_colorAnimator && m_colorAnimator->isRunning()) {
            m_colorAnimator->stop();
        }
        LFBox::setBackgroundColor(color);
    }
}

void LFButton::setText(const std::string& text) {
    if (!m_textNode) {
        m_textNode = std::make_shared<LFText>();
        // 默认文字样式
        m_textNode->setFontSize(16);
        m_textNode->setTextColor(0xFF000000);
        m_textNode->setTextHAlign(LFTextHAlign::Center);
        m_textNode->setTextVAlign(LFTextVAlign::Center);
        addChild(m_textNode, LFBoxAlign::Center);
    }
    m_textNode->setText(text);
}

void LFButton::setTextColor(uint32_t color) {
    if (m_textNode) m_textNode->setTextColor(color);
}

void LFButton::setFontSize(float size) {
    if (m_textNode) m_textNode->setFontSize(size);
}

// ==========================================
// 核心状态机与动画
// ==========================================

void LFButton::updateState(LFButtonState newState, bool animate) {
    if (m_state == newState) return;

    m_state = newState;

    // 1. 计算目标属性
    float targetScale = 1.0f;
    uint32_t targetColor = m_stateColors[LFButtonState::Normal];

    if (m_state == LFButtonState::Pressed) {
        targetScale = 0.95f; // 按下缩小
        targetColor = m_stateColors[LFButtonState::Pressed];
        // 如果没有专门设置 Pressed 颜色，可以自动变暗一点 (可选)
        if (targetColor == 0x00000000 && m_stateColors[LFButtonState::Normal] != 0) {
             // 简单的自动变暗逻辑 (Todo)
             targetColor = m_stateColors[LFButtonState::Normal];
        }
    } else if (m_state == LFButtonState::Disabled) {
        targetColor = m_stateColors[LFButtonState::Disabled];
    } else {
        targetColor = m_stateColors[LFButtonState::Normal];
    }

    // 2. 执行效果
    if (animate) {
        // 缩放动画 (仅当开启 Scale Effect 时)
        if (m_clickEffect == LFClickEffect::Scale) {
            runScaleAnimation(targetScale);
        } else {
            setScale(targetScale, targetScale);
        }

        // 颜色动画 (始终开启)
        runColorAnimation(targetColor);
    } else {
        // 无动画，直接设置
        setScale(targetScale, targetScale);
        LFBox::setBackgroundColor(targetColor);
    }
}

void LFButton::runScaleAnimation(float targetScale) {
    // 获取当前实际缩放值 (作为动画起点，防止跳变)
    float currentScale = getTransform().scaleX;

    // 创建或复用 Animator
    // 注意：为了物理手感，我们每次都创建新的，或者重置旧的
    // 这里使用我们之前实现的 LFValueAnimator<float>

    // 如果之前的动画还在跑，先停掉
    if (m_scaleAnimator && m_scaleAnimator->isRunning()) {
        m_scaleAnimator->stop();
    }

    m_scaleAnimator = LFValueAnimator<float>::of(currentScale, targetScale);

    // 关键：使用 Spring 物理效果
    // Damping 0.5 (Q弹), Freq 0.8 (响应速度)
    m_scaleAnimator->setSpring(0.5, 0.8);

    // 捕获 weak_this 防止循环引用
    std::weak_ptr<LFNode> weakSelf = shared_from_this();
    m_scaleAnimator->addUpdateListener([weakSelf](const float& val) {
        if (auto self = weakSelf.lock()) {
            self->setScale(val, val);
        }
    });

    m_scaleAnimator->start();

    // 注册到引擎
    LFGlobalAnimationManager::getInstance().addAnimator(m_scaleAnimator);
}

void LFButton::runColorAnimation(uint32_t targetColor) {
    uint32_t currentColor = getBackgroundColor();
    if (currentColor == targetColor) return;

    if (m_colorAnimator && m_colorAnimator->isRunning()) {
        m_colorAnimator->stop();
    }

    m_colorAnimator = LFValueAnimator<uint32_t>::of(currentColor, targetColor);
    // 颜色变化不需要物理回弹，用普通 Easing 更好
    m_colorAnimator->setDuration(0.15f); // 150ms 快速响应
    m_colorAnimator->setEasing(LFEasingType::QuadOut);

    std::weak_ptr<LFNode> weakSelf = shared_from_this();
    m_colorAnimator->addUpdateListener([weakSelf](const uint32_t& val) {
        if (auto self = std::static_pointer_cast<LFButton>(weakSelf.lock())) {
            // 注意这里调用父类 setBackgroundColor 避免递归
            // 其实 LFButton::setBackgroundColor 是设置配置，LFNode::set 是设置渲染属性
            // 所以这里应该调用 LFNode 的 set
            self->LFNode::setBackgroundColor(val);
        }
    });

    m_colorAnimator->start();
    LFGlobalAnimationManager::getInstance().addAnimator(m_colorAnimator);
}
