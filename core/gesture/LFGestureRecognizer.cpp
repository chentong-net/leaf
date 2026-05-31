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

            if (m_eventFilter && !m_eventFilter(event)) {
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

// ==========================================
// Pinch Gesture Recognizer Implementation
// ==========================================

void LFPinchGestureRecognizer::handleEvent(const LFTouchEvent& event) {
    // Pinch requires exactly 2 touch points
    switch (event.type) {
        case LFTouchEventType::Down: {
            // Track all touch points
            for (auto id : event.changedTouches) {
                startTrackingPointer(id);
            }

            // Check if we have exactly 2 pointers
            if (m_trackingPointers.size() == 2) {
                // Get two touch points
                auto it = m_trackingPointers.begin();
                LFTouchID id1 = *it++;
                LFTouchID id2 = *it;

                auto touch1 = event.getTouchById(id1);
                auto touch2 = event.getTouchById(id2);

                if (touch1 && touch2) {
                    LFPoint p1(touch1->x, touch1->y);
                    LFPoint p2(touch2->x, touch2->y);

                    m_initialDistance = calculateDistance(p1, p2);
                    m_previousDistance = m_initialDistance;
                    m_initialFocal = calculateFocal(p1, p2);
                    m_started = false;
                    setState(LFGestureState::Possible);

                    // Add to arena with first pointer ID
                    addToArena(id1);
                }
            } else if (m_trackingPointers.size() > 2) {
                // Too many pointers, fail
                setState(LFGestureState::Failed);
                for (auto id : m_trackingPointers) {
                    resolve(id, LFGestureDisposition::Rejected);
                }
                reset();
            }
            break;
        }

        case LFTouchEventType::Move: {
            if (m_trackingPointers.size() != 2) return;

            // Get two touch points
            auto it = m_trackingPointers.begin();
            LFTouchID id1 = *it++;
            LFTouchID id2 = *it;

            auto touch1 = event.getTouchById(id1);
            auto touch2 = event.getTouchById(id2);

            if (!touch1 || !touch2) return;

            LFPoint p1(touch1->x, touch1->y);
            LFPoint p2(touch2->x, touch2->y);

            float currentDistance = calculateDistance(p1, p2);
            LFPoint focal = calculateFocal(p1, p2);

            // Calculate scale (relative to previous frame)
            float scale = currentDistance / m_previousDistance;

            if (!m_started) {
                // Start pinch if distance changed significantly
                float totalScale = currentDistance / m_initialDistance;
                if (std::abs(totalScale - 1.0f) > 0.05f) { // 5% threshold
                    m_started = true;
                    setState(LFGestureState::Began);

                    // Accept in arena
                    resolve(id1, LFGestureDisposition::Accepted);

                    if (m_onPinchStart) {
                        m_onPinchStart(scale, focal);
                    }
                }
            }

            if (m_started) {
                setState(LFGestureState::Changed);

                if (m_onPinchUpdate) {
                    m_onPinchUpdate(scale, focal);
                }
            }

            m_previousDistance = currentDistance;
            break;
        }

        case LFTouchEventType::Up: {
            // Remove released pointer
            for (auto id : event.changedTouches) {
                stopTrackingPointer(id);
            }

            // If we still have 2 pointers, continue
            if (m_trackingPointers.size() == 2) {
                // Re-initialize for remaining pointers
                auto it = m_trackingPointers.begin();
                LFTouchID id1 = *it++;
                LFTouchID id2 = *it;

                auto touch1 = event.getTouchById(id1);
                auto touch2 = event.getTouchById(id2);

                if (touch1 && touch2) {
                    LFPoint p1(touch1->x, touch1->y);
                    LFPoint p2(touch2->x, touch2->y);
                    m_previousDistance = calculateDistance(p1, p2);
                }
            } else {
                // Less than 2 pointers, gesture ended
                if (m_started) {
                    setState(LFGestureState::Ended);

                    if (m_onPinchEnd) {
                        m_onPinchEnd(1.0f, m_initialFocal);
                    }
                } else {
                    setState(LFGestureState::Failed);
                    for (auto id : m_trackingPointers) {
                        resolve(id, LFGestureDisposition::Rejected);
                    }
                }
                reset();
            }
            break;
        }

        case LFTouchEventType::Cancel: {
            if (isTracking()) {
                setState(LFGestureState::Cancelled);
                for (auto id : m_trackingPointers) {
                    resolve(id, LFGestureDisposition::Rejected);
                }
                reset();
            }
            break;
        }
    }
}

float LFPinchGestureRecognizer::calculateDistance(const LFPoint& p1, const LFPoint& p2) {
    return p1.distance(p2);
}

LFPoint LFPinchGestureRecognizer::calculateFocal(const LFPoint& p1, const LFPoint& p2) {
    return LFPoint((p1.x + p2.x) / 2.0f, (p1.y + p2.y) / 2.0f);
}

void LFPinchGestureRecognizer::reset() {
    LFGestureRecognizer::reset();
    m_started = false;
    m_initialDistance = 0.0f;
    m_previousDistance = 0.0f;
}

// ==========================================
// Rotate Gesture Recognizer Implementation
// ==========================================

void LFRotateGestureRecognizer::handleEvent(const LFTouchEvent& event) {
    // Rotate requires exactly 2 touch points
    switch (event.type) {
        case LFTouchEventType::Down: {
            // Track all touch points
            for (auto id : event.changedTouches) {
                startTrackingPointer(id);
            }

            // Check if we have exactly 2 pointers
            if (m_trackingPointers.size() == 2) {
                auto it = m_trackingPointers.begin();
                LFTouchID id1 = *it++;
                LFTouchID id2 = *it;

                auto touch1 = event.getTouchById(id1);
                auto touch2 = event.getTouchById(id2);

                if (touch1 && touch2) {
                    LFPoint p1(touch1->x, touch1->y);
                    LFPoint p2(touch2->x, touch2->y);

                    m_initialAngle = calculateAngle(p1, p2);
                    m_previousAngle = m_initialAngle;
                    m_initialFocal = calculateFocal(p1, p2);
                    m_started = false;
                    setState(LFGestureState::Possible);

                    // Add to arena
                    addToArena(id1);
                }
            } else if (m_trackingPointers.size() > 2) {
                setState(LFGestureState::Failed);
                for (auto id : m_trackingPointers) {
                    resolve(id, LFGestureDisposition::Rejected);
                }
                reset();
            }
            break;
        }

        case LFTouchEventType::Move: {
            if (m_trackingPointers.size() != 2) return;

            auto it = m_trackingPointers.begin();
            LFTouchID id1 = *it++;
            LFTouchID id2 = *it;

            auto touch1 = event.getTouchById(id1);
            auto touch2 = event.getTouchById(id2);

            if (!touch1 || !touch2) return;

            LFPoint p1(touch1->x, touch1->y);
            LFPoint p2(touch2->x, touch2->y);

            float currentAngle = calculateAngle(p1, p2);
            LFPoint focal = calculateFocal(p1, p2);

            // Calculate rotation delta (in radians)
            float angleDelta = currentAngle - m_previousAngle;

            // Normalize to [-PI, PI]
            while (angleDelta > M_PI) angleDelta -= 2 * M_PI;
            while (angleDelta < -M_PI) angleDelta += 2 * M_PI;

            if (!m_started) {
                // Start rotation if angle changed significantly
                float totalRotation = currentAngle - m_initialAngle;
                while (totalRotation > M_PI) totalRotation -= 2 * M_PI;
                while (totalRotation < -M_PI) totalRotation += 2 * M_PI;

                if (std::abs(totalRotation) > 0.1f) { // ~5.7 degrees threshold
                    m_started = true;
                    setState(LFGestureState::Began);

                    // Accept in arena
                    resolve(id1, LFGestureDisposition::Accepted);

                    if (m_onRotateStart) {
                        m_onRotateStart(angleDelta, focal);
                    }
                }
            }

            if (m_started) {
                setState(LFGestureState::Changed);

                if (m_onRotateUpdate) {
                    m_onRotateUpdate(angleDelta, focal);
                }
            }

            m_previousAngle = currentAngle;
            break;
        }

        case LFTouchEventType::Up: {
            for (auto id : event.changedTouches) {
                stopTrackingPointer(id);
            }

            if (m_trackingPointers.size() == 2) {
                // Re-initialize
                auto it = m_trackingPointers.begin();
                LFTouchID id1 = *it++;
                LFTouchID id2 = *it;

                auto touch1 = event.getTouchById(id1);
                auto touch2 = event.getTouchById(id2);

                if (touch1 && touch2) {
                    LFPoint p1(touch1->x, touch1->y);
                    LFPoint p2(touch2->x, touch2->y);
                    m_previousAngle = calculateAngle(p1, p2);
                }
            } else {
                if (m_started) {
                    setState(LFGestureState::Ended);

                    if (m_onRotateEnd) {
                        m_onRotateEnd(0.0f, m_initialFocal);
                    }
                } else {
                    setState(LFGestureState::Failed);
                    for (auto id : m_trackingPointers) {
                        resolve(id, LFGestureDisposition::Rejected);
                    }
                }
                reset();
            }
            break;
        }

        case LFTouchEventType::Cancel: {
            if (isTracking()) {
                setState(LFGestureState::Cancelled);
                for (auto id : m_trackingPointers) {
                    resolve(id, LFGestureDisposition::Rejected);
                }
                reset();
            }
            break;
        }
    }
}

float LFRotateGestureRecognizer::calculateAngle(const LFPoint& p1, const LFPoint& p2) {
    // Calculate angle from p1 to p2 (in radians)
    return std::atan2(p2.y - p1.y, p2.x - p1.x);
}

LFPoint LFRotateGestureRecognizer::calculateFocal(const LFPoint& p1, const LFPoint& p2) {
    return LFPoint((p1.x + p2.x) / 2.0f, (p1.y + p2.y) / 2.0f);
}

void LFRotateGestureRecognizer::reset() {
    LFGestureRecognizer::reset();
    m_started = false;
    m_initialAngle = 0.0f;
    m_previousAngle = 0.0f;
}

// ==========================================
// Swipe Gesture Recognizer Implementation
// ==========================================

void LFSwipeGestureRecognizer::handleEvent(const LFTouchEvent& event) {
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
            m_startTime = event.timestamp;
            m_velocity = LFPoint(0, 0);
            setState(LFGestureState::Possible);

            // Add to arena
            addToArena(touch->id);
            break;
        }

        case LFTouchEventType::Move: {
            if (!isTrackingPointer(touch->id)) return;

            // Calculate velocity
            LFPoint currentPos(touch->x, touch->y);
            LFPoint delta = currentPos - m_startPosition;
            double dt = event.timestamp - m_startTime;

            if (dt > 0) {
                m_velocity.x = delta.x / dt;
                m_velocity.y = delta.y / dt;
            }
            break;
        }

        case LFTouchEventType::Up: {
            if (!isTrackingPointer(touch->id)) return;

            LFPoint currentPos(touch->x, touch->y);
            LFPoint delta = currentPos - m_startPosition;
            double duration = event.timestamp - m_startTime;

            // Check if swipe conditions met
            float speed = std::sqrt(m_velocity.x * m_velocity.x + m_velocity.y * m_velocity.y);

            if (speed >= m_minVelocity && duration <= m_maxDuration) {
                // Detect direction
                SwipeDirection direction = detectDirection(delta, m_velocity);

                if (direction != SwipeDirection::None) {
                    setState(LFGestureState::Recognized);
                    resolve(touch->id, LFGestureDisposition::Accepted);

                    if (m_onSwipe) {
                        m_onSwipe(direction, m_velocity);
                    }
                } else {
                    setState(LFGestureState::Failed);
                    resolve(touch->id, LFGestureDisposition::Rejected);
                }
            } else {
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

LFSwipeGestureRecognizer::SwipeDirection LFSwipeGestureRecognizer::detectDirection(
    const LFPoint& delta,
    const LFPoint& velocity
) {
    // Determine primary direction based on velocity
    float absVx = std::abs(velocity.x);
    float absVy = std::abs(velocity.y);

    SwipeDirection direction = SwipeDirection::None;

    if (absVx > absVy) {
        // Horizontal swipe
        if (velocity.x > 0) {
            direction = SwipeDirection::Right;
        } else {
            direction = SwipeDirection::Left;
        }
    } else {
        // Vertical swipe
        if (velocity.y > 0) {
            direction = SwipeDirection::Down;
        } else {
            direction = SwipeDirection::Up;
        }
    }

    // Check if direction is allowed
    if ((int)direction & m_allowedDirections) {
        return direction;
    }

    return SwipeDirection::None;
}

void LFSwipeGestureRecognizer::reset() {
    LFGestureRecognizer::reset();
    m_velocity = LFPoint(0, 0);
}
