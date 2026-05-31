//
// Created by Chen Tong on 2026/1/24.
//

//
// Created by Leaf Engine Team.
// UI Component - Button Implementation
//

#include "LFButton.h"
#include "LFEngine.h"

#include <cmath>

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
        if (const LFTouchPoint* touch = e.getPrimaryTouch()) {
            m_touchDownX = touch->x;
            m_touchDownY = touch->y;
        }

        // 进入按下状态
        updateState(LFButtonState::Pressed);
    });

    // 2. 移动 (TouchMove) - 实现“移出取消、移回恢复”
    setOnTouchMove([this](const LFTouchEvent& e) {
        if (!m_isEnabled || !m_isTouching) return;

        const LFTouchPoint* touch = e.getPrimaryTouch();
        if (!touch) return;
        float dx = touch->x - m_touchDownX;
        float dy = touch->y - m_touchDownY;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > m_touchSlop) {
            m_isInside = false;
            if (m_state == LFButtonState::Pressed) {
                updateState(LFButtonState::Normal);
            }
        }
    });

    // 3. 抬起 (TouchUp)
    setOnTouchUp([this](const LFTouchEvent& e) {
        if (!m_isEnabled || !m_isTouching) return;

        m_isTouching = false;

        bool validClick = false;

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

void LFButton::setClickEffect(LFClickEffect effect) {
    if (m_clickEffect == effect) return;

    m_clickEffect = effect;

    if (m_clickEffect != LFClickEffect::Scale) {
        if (m_scaleAnimator && m_scaleAnimator->isRunning()) {
            m_scaleAnimator->stop();
        }
        setScale(1.0f, 1.0f);
    }
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
            if (m_scaleAnimator && m_scaleAnimator->isRunning()) {
                m_scaleAnimator->stop();
            }
            setScale(1.0f, 1.0f);
        }

        // 颜色动画 (始终开启)
        runColorAnimation(targetColor);
    } else {
        // 无动画，直接设置
        if (m_clickEffect == LFClickEffect::Scale) {
            setScale(targetScale, targetScale);
        } else {
            if (m_scaleAnimator && m_scaleAnimator->isRunning()) {
                m_scaleAnimator->stop();
            }
            setScale(1.0f, 1.0f);
        }
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
