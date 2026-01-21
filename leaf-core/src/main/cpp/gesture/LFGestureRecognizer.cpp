//
// Created by Chen Tong on 2026/1/21.
// Gesture System - Gesture Recognizer Implementation
//

#include "LFGestureRecognizer.h"
#include "../event/LFGestureArena.h"
#include <cmath>
#include <algorithm>

// ==========================================
// Base Gesture Recognizer Implementation
// ==========================================

void LFGestureRecognizer::acceptGesture(int pointer) {
    // Called when this recognizer wins the arena
    // Subclasses can override to handle acceptance
}

void LFGestureRecognizer::rejectGesture(int pointer) {
    // Called when this recognizer loses the arena
    // Reset state
    setState(LFGestureState::Failed);
    stopTrackingPointer(pointer);
}

void LFGestureRecognizer::addToArena(int pointer) {
    // Add this recognizer to the arena for the given pointer
    auto& manager = LFGestureArenaManager::getInstance();
    auto entry = manager.add(pointer, shared_from_this());
    if (entry) {
        m_arenaEntries[pointer] = entry;
    }
}

void LFGestureRecognizer::resolve(int pointer, LFGestureDisposition disposition) {
    // Resolve this recognizer's participation in the arena
    auto it = m_arenaEntries.find(pointer);
    if (it != m_arenaEntries.end()) {
        it->second->resolve(disposition);
    }

    // Trigger arena sweep
    auto& manager = LFGestureArenaManager::getInstance();
    manager.sweep(pointer);
}

// ==========================================
// Tap Gesture Recognizer Implementation
// ==========================================

void LFTapGestureRecognizer::handleEvent(const LFTouchEvent& event) {
    if (event.touches.empty()) return;

    const LFTouchPoint* touch = event.getPrimaryTouch();
    if (!touch) return;

    switch (event.type) {
        case LFTouchEventType::Down: {
            if (isTracking()) {
                // Already tracking another touch, fail
                setState(LFGestureState::Failed);
                resolve(touch->id, LFGestureDisposition::Rejected);
                return;
            }

            startTrackingPointer(touch->id);
            m_startPosition = LFPoint(touch->x, touch->y);
            m_downTime = event.timestamp;
            setState(LFGestureState::Possible);

            // Add to arena
            addToArena(touch->id);
            break;
        }

        case LFTouchEventType::Move: {
            if (!isTrackingPointer(touch->id)) return;

            // Check movement distance
            float distance = m_startPosition.distance(LFPoint(touch->x, touch->y));
            if (distance > m_maxTapDistance) {
                // Moved too far, recognition failed
                setState(LFGestureState::Failed);
                resolve(touch->id, LFGestureDisposition::Rejected);
                stopTrackingPointer(touch->id);
            }
            break;
        }

        case LFTouchEventType::Up: {
            if (!isTrackingPointer(touch->id)) return;

            double duration = event.timestamp - m_downTime;
            float distance = m_startPosition.distance(LFPoint(touch->x, touch->y));

            if (duration <= m_maxTapDuration && distance <= m_maxTapDistance) {
                // Recognition successful
                setState(LFGestureState::Recognized);
                resolve(touch->id, LFGestureDisposition::Accepted);

                // Check double tap
                if (m_doubleTapEnabled) {
                    double tapInterval = event.timestamp - m_lastTapTime;
                    float tapDistance = m_startPosition.distance(m_lastTapPosition);

                    if (m_tapCount == 1 &&
                        tapInterval <= m_doubleTapTimeout &&
                        tapDistance <= m_maxTapDistance) {
                        // Double tap successful
                        if (m_onDoubleTap) {
                            m_onDoubleTap(LFPoint(touch->x, touch->y));
                        }
                        m_tapCount = 0;
                        m_lastTapTime = 0.0;
                    } else {
                        // Single tap (wait for possible double tap)
                        m_tapCount = 1;
                        m_lastTapTime = event.timestamp;
                        m_lastTapPosition = m_startPosition;

                        // Trigger single tap callback immediately
                        // (In production, should delay to wait for double tap)
                        if (m_onTap) {
                            m_onTap(LFPoint(touch->x, touch->y));
                        }
                    }
                } else {
                    // No double tap support, trigger immediately
                    if (m_onTap) {
                        m_onTap(LFPoint(touch->x, touch->y));
                    }
                }
            } else {
                // Recognition failed
                setState(LFGestureState::Failed);
                resolve(touch->id, LFGestureDisposition::Rejected);
            }

            stopTrackingPointer(touch->id);
            break;
        }

        case LFTouchEventType::Cancel: {
            if (isTrackingPointer(touch->id)) {
                setState(LFGestureState::Cancelled);
                resolve(touch->id, LFGestureDisposition::Rejected);
                stopTrackingPointer(touch->id);
            }
            break;
        }
    }
}

void LFTapGestureRecognizer::reset() {
    LFGestureRecognizer::reset();
    m_downTime = 0.0;
}

// ==========================================
// Long Press Gesture Recognizer Implementation
// ==========================================

void LFLongPressGestureRecognizer::handleEvent(const LFTouchEvent& event) {
    if (event.touches.empty()) return;

    const LFTouchPoint* touch = event.getPrimaryTouch();
    if (!touch) return;

    switch (event.type) {
        case LFTouchEventType::Down: {
            if (isTracking()) {
                setState(LFGestureState::Failed);
                resolve(touch->id, LFGestureDisposition::Rejected);
                return;
            }

            startTrackingPointer(touch->id);
            m_startPosition = LFPoint(touch->x, touch->y);
            m_downTime = event.timestamp;
            m_triggered = false;
            setState(LFGestureState::Possible);

            // Add to arena
            addToArena(touch->id);
            break;
        }

        case LFTouchEventType::Move: {
            if (!isTrackingPointer(touch->id)) return;

            // Check movement distance
            float distance = m_startPosition.distance(LFPoint(touch->x, touch->y));
            if (distance > m_maxMoveDistance) {
                // Moved too far, fail
                setState(LFGestureState::Failed);
                resolve(touch->id, LFGestureDisposition::Rejected);
                stopTrackingPointer(touch->id);
                m_triggered = false;
            }
            break;
        }

        case LFTouchEventType::Up: {
            if (!isTrackingPointer(touch->id)) return;

            if (!m_triggered) {
                // Released before long press duration, fail
                setState(LFGestureState::Failed);
                resolve(touch->id, LFGestureDisposition::Rejected);
            } else {
                setState(LFGestureState::Ended);
            }

            stopTrackingPointer(touch->id);
            m_triggered = false;
            break;
        }

        case LFTouchEventType::Cancel: {
            if (isTrackingPointer(touch->id)) {
                setState(LFGestureState::Cancelled);
                resolve(touch->id, LFGestureDisposition::Rejected);
                stopTrackingPointer(touch->id);
                m_triggered = false;
            }
            break;
        }
    }
}

void LFLongPressGestureRecognizer::update(double currentTime) {
    if (!isTracking() || m_triggered) return;

    // Check if duration reached
    if (currentTime - m_downTime >= m_minPressDuration) {
        m_triggered = true;
        setState(LFGestureState::Recognized);

        // Accept in arena (won over Tap)
        for (auto id : m_trackingPointers) {
            resolve(id, LFGestureDisposition::Accepted);
        }

        // Trigger callback
        if (m_onLongPress) {
            m_onLongPress(m_startPosition);
        }
    }
}

void LFLongPressGestureRecognizer::reset() {
    LFGestureRecognizer::reset();
    m_downTime = 0.0;
    m_triggered = false;
}

// ==========================================
// Pan Gesture Recognizer Implementation
// ==========================================

void LFPanGestureRecognizer::handleEvent(const LFTouchEvent& event) {
    if (event.touches.empty()) return;

    const LFTouchPoint* touch = event.getPrimaryTouch();
    if (!touch) return;

    switch (event.type) {
        case LFTouchEventType::Down: {
            if (isTracking()) {
                setState(LFGestureState::Failed);
                resolve(touch->id, LFGestureDisposition::Rejected);
                return;
            }

            startTrackingPointer(touch->id);
            m_startPosition = LFPoint(touch->x, touch->y);
            m_lastPosition = m_startPosition;
            m_lastTimestamp = event.timestamp;
            m_started = false;
            m_velocity = LFPoint(0, 0);
            setState(LFGestureState::Possible);

            // Add to arena
            addToArena(touch->id);
            break;
        }

        case LFTouchEventType::Move: {
            if (!isTrackingPointer(touch->id)) return;

            LFPoint currentPos(touch->x, touch->y);
            LFPoint totalDelta = currentPos - m_startPosition;
            float distance = m_startPosition.distance(currentPos);

            if (!m_started) {
                // Check if minimum distance reached
                if (distance >= m_minDistance) {
                    // Check direction constraint
                    if (checkDirection(totalDelta)) {
                        m_started = true;
                        setState(LFGestureState::Began);

                        // Accept in arena (won over Tap)
                        resolve(touch->id, LFGestureDisposition::Accepted);

                        // Trigger start callback
                        if (m_onPanStart) {
                            LFPoint delta = currentPos - m_lastPosition;
                            m_onPanStart(delta, m_velocity);
                        }
                    } else {
                        // Direction mismatch, fail
                        setState(LFGestureState::Failed);
                        resolve(touch->id, LFGestureDisposition::Rejected);
                        stopTrackingPointer(touch->id);
                        return;
                    }
                }
            }

            if (m_started) {
                setState(LFGestureState::Changed);

                // Calculate delta and velocity
                LFPoint delta = currentPos - m_lastPosition;
                double dt = event.timestamp - m_lastTimestamp;
                updateVelocity(delta, dt);

                // Trigger update callback
                if (m_onPanUpdate) {
                    m_onPanUpdate(delta, m_velocity);
                }

                m_lastPosition = currentPos;
                m_lastTimestamp = event.timestamp;
            }
            break;
        }

        case LFTouchEventType::Up: {
            if (!isTrackingPointer(touch->id)) return;

            if (m_started) {
                setState(LFGestureState::Ended);

                // Trigger end callback
                if (m_onPanEnd) {
                    LFPoint delta = LFPoint(touch->x, touch->y) - m_lastPosition;
                    m_onPanEnd(delta, m_velocity);
                }
            } else {
                setState(LFGestureState::Failed);
                resolve(touch->id, LFGestureDisposition::Rejected);
            }

            stopTrackingPointer(touch->id);
            m_started = false;
            break;
        }

        case LFTouchEventType::Cancel: {
            if (isTrackingPointer(touch->id)) {
                setState(LFGestureState::Cancelled);
                resolve(touch->id, LFGestureDisposition::Rejected);
                stopTrackingPointer(touch->id);
                m_started = false;
            }
            break;
        }
    }
}

bool LFPanGestureRecognizer::checkDirection(const LFPoint& delta) {
    if (m_direction == PanDirection::Any) {
        return true;
    }

    float absDeltaX = std::abs(delta.x);
    float absDeltaY = std::abs(delta.y);

    if (m_direction == PanDirection::Horizontal) {
        return absDeltaX > absDeltaY;
    } else if (m_direction == PanDirection::Vertical) {
        return absDeltaY > absDeltaX;
    }

    return true;
}

void LFPanGestureRecognizer::updateVelocity(const LFPoint& delta, double dt) {
    if (dt > 0) {
        // Simple velocity calculation (pixels per second)
        float vx = delta.x / dt;
        float vy = delta.y / dt;

        // Apply smoothing (exponential moving average)
        const float alpha = 0.3f;
        m_velocity.x = alpha * vx + (1.0f - alpha) * m_velocity.x;
        m_velocity.y = alpha * vy + (1.0f - alpha) * m_velocity.y;
    }
}

void LFPanGestureRecognizer::reset() {
    LFGestureRecognizer::reset();
    m_started = false;
    m_velocity = LFPoint(0, 0);
}
