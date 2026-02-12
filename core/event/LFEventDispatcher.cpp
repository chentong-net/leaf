//
// Created by Chen Tong on 2026/1/21.
// Event System - Event Dispatcher Implementation
//

#include "LFEventDispatcher.h"
#include "LFGestureArena.h"
#include "../gesture/LFGestureRecognizer.h"
#include <algorithm>

namespace {

bool isSameOrDescendant(const std::shared_ptr<LFNode>& node, const std::shared_ptr<LFNode>& ancestor) {
    if (!node || !ancestor) return false;
    LFNode* current = node.get();
    while (current) {
        if (current == ancestor.get()) {
            return true;
        }
        current = current->getParent();
    }
    return false;
}

}

LFEventDispatcher& LFEventDispatcher::getInstance() {
    static LFEventDispatcher instance;
    return instance;
}

void LFEventDispatcher::dispatchTouchEvent(
    LFTouchEventType type,
    const std::vector<LFTouchPoint>& touches,
    const std::vector<LFTouchID>& changedIDs,
    std::shared_ptr<LFNode> root
) {
    if (!root || touches.empty()) return;

    // 1. Build event object
    LFTouchEvent event;
    event.type = type;
    event.touches = touches;
    event.changedTouches = changedIDs;
    event.timestamp = touches.empty() ? 0.0 : touches[0].timestamp;

    // 2. Update active touch point state
    for (auto id : changedIDs) {
        auto it = std::find_if(touches.begin(), touches.end(),
            [id](const LFTouchPoint& t) { return t.id == id; });

        if (it != touches.end()) {
            if (type == LFTouchEventType::Down) {
                m_activeTouches[id] = *it;
            } else if (type == LFTouchEventType::Up || type == LFTouchEventType::Cancel) {
                m_activeTouches.erase(id);
            } else {
                m_activeTouches[id] = *it;
            }
        }
    }

    // 3. Perform HitTest (only for Down event)
    if (type == LFTouchEventType::Down) {
        for (auto id : changedIDs) {
            auto it = std::find_if(touches.begin(), touches.end(),
                [id](const LFTouchPoint& t) { return t.id == id; });

            if (it != touches.end()) {
                LFHitTestResult hitResult = LFHitTestEngine::hitTest(
                    root, it->x, it->y
                );

                if (hitResult.hasTarget()) {
                    m_touchTargets[id] = hitResult.getTarget();
                    event.target = hitResult.getTarget();
                } else {
                    // No target found, event goes nowhere
                    event.target.reset();
                }
            }
        }
    } else {
        // Move/Up/Cancel use cached target
        for (auto id : changedIDs) {
            auto targetIt = m_touchTargets.find(id);
            if (targetIt != m_touchTargets.end()) {
                event.target = targetIt->second;
                break; // Use first valid target
            }
        }
    }

    if (type == LFTouchEventType::Down) {
        auto focused = getFocusedNode();
        auto target = event.target.lock();
        if (focused && !isSameOrDescendant(target, focused)) {
            clearFocus();
        }
    }

    // 4. Execute three-phase dispatch
    if (event.target.lock()) {
        performDispatch(event, root);
    }

    // 5. Close arena and cleanup (Up/Cancel events)
    if (type == LFTouchEventType::Up || type == LFTouchEventType::Cancel) {
        auto& arenaManager = LFGestureArenaManager::getInstance();
        for (auto id : changedIDs) {
            // Close arena for this pointer (no more members can join)
            arenaManager.close(id);
            m_touchTargets.erase(id);
        }
    }
}

void LFEventDispatcher::dispatchKeyEvent(LFKeyEventType type, LFKeyCode keyCode, uint32_t modifiers, bool repeat) {
    auto focused = m_focusedNode.lock();
    if (!focused) return;

    LFKeyEvent event;
    event.type = type;
    event.keyCode = keyCode;
    event.modifiers = modifiers;
    event.repeat = repeat;
    event.target = focused;
    event.currentTarget = focused;

    invokeKeyListener(focused, event);
}

void LFEventDispatcher::dispatchCharInput(uint32_t codepoint) {
    auto focused = m_focusedNode.lock();
    if (!focused) return;

    LFKeyEvent event;
    event.type = LFKeyEventType::Char;
    event.codepoint = codepoint;
    event.keyCode = LFKeyCode::Unknown;
    event.target = focused;
    event.currentTarget = focused;

    invokeKeyListener(focused, event);
}

void LFEventDispatcher::setFocusedNode(const std::shared_ptr<LFNode>& node) {
    if (node && !node->isFocusable()) return;

    auto oldFocused = m_focusedNode.lock();
    if (oldFocused == node) return;

    if (oldFocused) {
        oldFocused->setFocusState(false);
    }

    if (node) {
        m_focusedNode = node;
        node->setFocusState(true);
    } else {
        m_focusedNode.reset();
    }
}

void LFEventDispatcher::clearFocus(const std::shared_ptr<LFNode>& expectedNode) {
    auto focused = m_focusedNode.lock();
    if (!focused) return;
    if (expectedNode && expectedNode != focused) return;

    focused->setFocusState(false);
    m_focusedNode.reset();
}

std::shared_ptr<LFNode> LFEventDispatcher::getFocusedNode() const {
    return m_focusedNode.lock();
}

void LFEventDispatcher::performDispatch(
    LFTouchEvent& event,
    std::shared_ptr<LFNode> root
) {
    auto target = event.target.lock();
    if (!target) return;

    // Build path from root to target
    std::vector<std::shared_ptr<LFNode>> path = buildEventPath(root, target);

    // 1. Capture phase (from root to target, excluding target)
    capturePhase(event, path);

    // 2. Target phase
    if (!event.propagationStopped && target) {
        targetPhase(event, target);
    }

    // 3. Bubble phase (from target to root, excluding target)
    if (!event.propagationStopped) {
        bubblePhase(event, path);
    }
}

void LFEventDispatcher::capturePhase(
    LFTouchEvent& event,
    const std::vector<std::shared_ptr<LFNode>>& path
) {
    event.phase = LFEventPhase::Capture;

    // From root to target (not including target)
    for (size_t i = 0; i < path.size() - 1; ++i) {
        if (event.propagationStopped) break;

        auto node = path[i];
        event.currentTarget = node;

        // Check if parent wants to intercept
        auto interceptListener = node->getOnInterceptTouchEvent();
        if (interceptListener) {
            if (interceptListener(event)) {
                // Parent intercepted, change target to parent
                event.target = node;

                // Truncate path to parent
                // We'll skip remaining capture and go straight to target phase on parent
                event.propagationStopped = true;
                targetPhase(event, node);
                return;
            }
        }

        // Invoke capture listener
        invokeListener(node, event);
    }
}

void LFEventDispatcher::targetPhase(
    LFTouchEvent& event,
    std::shared_ptr<LFNode> target
) {
    event.phase = LFEventPhase::Target;
    event.currentTarget = target;
    invokeListener(target, event);
}

void LFEventDispatcher::bubblePhase(
    LFTouchEvent& event,
    const std::vector<std::shared_ptr<LFNode>>& path
) {
    event.phase = LFEventPhase::Bubble;

    // From parent of target to root (reverse order, excluding target)
    for (int i = (int)path.size() - 2; i >= 0; --i) {
        if (event.propagationStopped) break;

        auto node = path[i];
        event.currentTarget = node;
        invokeListener(node, event);
    }
}

void LFEventDispatcher::invokeListener(
    std::shared_ptr<LFNode> node,
    LFTouchEvent& event
) {
    if (!node) return;

    // 1. Dispatch to gesture recognizers (Phase 2)
    const auto& recognizers = node->getGestureRecognizers();
    for (auto& recognizer : recognizers) {
        if (recognizer) {
            recognizer->handleEvent(event);
        }
    }

    // 2. Call basic touch event listeners
    switch (event.type) {
        case LFTouchEventType::Down: {
            auto listener = node->getOnTouchDown();
            if (listener) listener(event);
            break;
        }
        case LFTouchEventType::Move: {
            auto listener = node->getOnTouchMove();
            if (listener) listener(event);
            break;
        }
        case LFTouchEventType::Up: {
            auto listener = node->getOnTouchUp();
            if (listener) listener(event);
            break;
        }
        case LFTouchEventType::Cancel: {
            auto listener = node->getOnTouchCancel();
            if (listener) listener(event);
            break;
        }
    }
}

void LFEventDispatcher::invokeKeyListener(
    std::shared_ptr<LFNode> node,
    LFKeyEvent& event
) {
    if (!node) return;

    switch (event.type) {
        case LFKeyEventType::Down: {
            auto listener = node->getOnKeyDown();
            if (listener) listener(event);
            break;
        }
        case LFKeyEventType::Up: {
            auto listener = node->getOnKeyUp();
            if (listener) listener(event);
            break;
        }
        case LFKeyEventType::Char: {
            auto listener = node->getOnCharInput();
            if (listener) listener(event);
            break;
        }
    }
}


std::vector<std::shared_ptr<LFNode>> LFEventDispatcher::buildEventPath(
    std::shared_ptr<LFNode> root,
    std::shared_ptr<LFNode> target
) {
    std::vector<std::shared_ptr<LFNode>> path;

    // Build path from target to root
    LFNode* current = target.get();
    while (current) {
        path.insert(path.begin(), current->shared_from_this());
        current = current->getParent();
    }

    return path;
}

void LFEventDispatcher::update(double currentTime, std::shared_ptr<LFNode> root) {
    if (!root) return;

    // Recursively update all gesture recognizers in the tree
    updateNodeGestures(root, currentTime);
}

void LFEventDispatcher::updateNodeGestures(std::shared_ptr<LFNode> node, double currentTime) {
    if (!node) return;

    // Update gesture recognizers for this node
    const auto& recognizers = node->getGestureRecognizers();
    for (auto& recognizer : recognizers) {
        if (recognizer) {
            recognizer->update(currentTime);
        }
    }

    // Recursively update children
    const auto& children = node->getChildren();
    for (const auto& child : children) {
        updateNodeGestures(child, currentTime);
    }
}
