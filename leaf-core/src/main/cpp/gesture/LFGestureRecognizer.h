//
// Created by Chen Tong on 2026/1/21.
// Gesture System - Base Gesture Recognizer
//

#ifndef LEAF_LFGESTURERECOGNIZER_H
#define LEAF_LFGESTURERECOGNIZER_H

#include "../event/LFEvent.h"
#include <functional>
#include <memory>
#include <set>

// Gesture recognizer state
enum class LFGestureState {
    Possible,       // Possible to recognize
    Began,          // Began
    Changed,        // Changing
    Ended,          // Ended
    Cancelled,      // Cancelled
    Failed,         // Recognition failed
    Recognized      // Recognized (for discrete gestures like Tap)
};

// Base class for all gesture recognizers (abstract)
class LFGestureRecognizer : public std::enable_shared_from_this<LFGestureRecognizer> {
public:
    using Ptr = std::shared_ptr<LFGestureRecognizer>;
    virtual ~LFGestureRecognizer() = default;

    // Core interface: subclasses must implement
    virtual void handleEvent(const LFTouchEvent& event) = 0;

    // Update method (for time-based gestures like LongPress)
    // Called by engine update loop
    virtual void update(double currentTime) {}

    // Reset state
    virtual void reset() {
        m_state = LFGestureState::Possible;
        m_trackingPointers.clear();
    }

    // State access
    LFGestureState getState() const { return m_state; }
    bool isTracking() const { return !m_trackingPointers.empty(); }

protected:
    // State update (called by subclasses)
    void setState(LFGestureState state) { m_state = state; }

    // Add/remove tracked touch points
    void startTrackingPointer(LFTouchID id) { m_trackingPointers.insert(id); }
    void stopTrackingPointer(LFTouchID id) { m_trackingPointers.erase(id); }
    bool isTrackingPointer(LFTouchID id) const {
        return m_trackingPointers.find(id) != m_trackingPointers.end();
    }

    LFGestureState m_state = LFGestureState::Possible;
    std::set<LFTouchID> m_trackingPointers;
};

// ==========================================
// Tap Gesture Recognizer (Click)
// ==========================================

class LFTapGestureRecognizer : public LFGestureRecognizer {
public:
    using TapCallback = std::function<void(const LFPoint& location)>;

    void setOnTap(TapCallback callback) { m_onTap = callback; }
    void setMaxTapDuration(float seconds) { m_maxTapDuration = seconds; }
    void setMaxTapDistance(float pixels) { m_maxTapDistance = pixels; }

    // Double tap support
    void setDoubleTapEnabled(bool enabled) { m_doubleTapEnabled = enabled; }
    void setDoubleTapTimeout(float seconds) { m_doubleTapTimeout = seconds; }
    void setOnDoubleTap(TapCallback callback) { m_onDoubleTap = callback; }

    void handleEvent(const LFTouchEvent& event) override;
    void reset() override;

private:
    TapCallback m_onTap;
    TapCallback m_onDoubleTap;

    float m_maxTapDuration = 0.3f;      // Max tap duration
    float m_maxTapDistance = 10.0f;     // Max movement distance

    bool m_doubleTapEnabled = false;
    float m_doubleTapTimeout = 0.3f;

    LFPoint m_startPosition;
    double m_downTime = 0.0;

    // Double tap state
    int m_tapCount = 0;
    double m_lastTapTime = 0.0;
    LFPoint m_lastTapPosition;
};

// ==========================================
// Long Press Gesture Recognizer
// ==========================================

class LFLongPressGestureRecognizer : public LFGestureRecognizer {
public:
    using LongPressCallback = std::function<void(const LFPoint& location)>;

    void setOnLongPress(LongPressCallback callback) { m_onLongPress = callback; }
    void setMinPressDuration(float seconds) { m_minPressDuration = seconds; }
    void setMaxMoveDistance(float pixels) { m_maxMoveDistance = pixels; }

    void handleEvent(const LFTouchEvent& event) override;
    void reset() override;

    // Called by engine update loop to check if long press duration reached
    void update(double currentTime) override;

private:
    LongPressCallback m_onLongPress;
    float m_minPressDuration = 0.5f;
    float m_maxMoveDistance = 10.0f;

    LFPoint m_startPosition;
    double m_downTime = 0.0;
    bool m_triggered = false;
};

// ==========================================
// Pan Gesture Recognizer (Drag/Swipe)
// ==========================================

class LFPanGestureRecognizer : public LFGestureRecognizer {
public:
    using PanCallback = std::function<void(const LFPoint& delta, const LFPoint& velocity)>;

    void setOnPanStart(PanCallback callback) { m_onPanStart = callback; }
    void setOnPanUpdate(PanCallback callback) { m_onPanUpdate = callback; }
    void setOnPanEnd(PanCallback callback) { m_onPanEnd = callback; }

    void setMinDistance(float pixels) { m_minDistance = pixels; }

    // Direction locking
    enum class PanDirection { Any, Horizontal, Vertical };
    void setDirection(PanDirection dir) { m_direction = dir; }

    void handleEvent(const LFTouchEvent& event) override;
    void reset() override;

private:
    PanCallback m_onPanStart;
    PanCallback m_onPanUpdate;
    PanCallback m_onPanEnd;

    float m_minDistance = 10.0f;
    PanDirection m_direction = PanDirection::Any;

    LFPoint m_startPosition;
    LFPoint m_lastPosition;
    double m_lastTimestamp = 0.0;
    LFPoint m_velocity;

    bool m_started = false;

    bool checkDirection(const LFPoint& delta);
    void updateVelocity(const LFPoint& delta, double dt);
};

#endif // LEAF_LFGESTURERECOGNIZER_H
