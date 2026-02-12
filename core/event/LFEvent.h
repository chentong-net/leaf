//
// Created by Chen Tong on 2026/1/21.
// Event System - Core Event Types Definition
//

#ifndef LEAF_LFEVENT_H
#define LEAF_LFEVENT_H

#include "../LFDef.h"
#include <memory>
#include <vector>
#include <cmath>

// Forward declaration
class LFNode;

// Touch ID type (for multi-touch support)
using LFTouchID = int32_t;

// Event phase (capture → target → bubble)
enum class LFEventPhase {
    None,       // No phase
    Capture,    // Capture phase (from root to target)
    Target,     // Target phase
    Bubble      // Bubble phase (from target to root)
};

// Touch event type
enum class LFTouchEventType {
    Down,       // Finger down
    Move,       // Finger move
    Up,         // Finger up
    Cancel      // Event cancelled (interrupted by system)
};

// Keyboard event type
enum class LFKeyEventType {
    Down,
    Up,
    Char
};

// Cross-platform key code
enum class LFKeyCode : int32_t {
    Unknown = 0,
    Enter = 13,
    Tab = 9,
    Backspace = 8,
    Escape = 27,
    Delete = 127,
    Left = 1001,
    Right = 1002,
    Up = 1003,
    Down = 1004,
    Home = 1005,
    End = 1006
};

enum LFKeyMod : uint32_t {
    LFKeyModNone = 0,
    LFKeyModShift = 1 << 0,
    LFKeyModCtrl = 1 << 1,
    LFKeyModAlt = 1 << 2,
    LFKeyModSuper = 1 << 3
};

// Touch point information
struct LFTouchPoint {
    LFTouchID id = 0;               // Touch point ID (for multi-touch)
    float x = 0.0f;                 // Screen coordinate X
    float y = 0.0f;                 // Screen coordinate Y
    float pressure = 1.0f;          // Pressure (0.0-1.0)
    double timestamp = 0.0;         // Timestamp (seconds)

    // Helper attributes
    float startX = 0.0f;            // Start coordinate
    float startY = 0.0f;
    float lastX = 0.0f;             // Last coordinate
    float lastY = 0.0f;

    // Calculate offsets
    float getDeltaX() const { return x - lastX; }
    float getDeltaY() const { return y - lastY; }
    float getTotalDeltaX() const { return x - startX; }
    float getTotalDeltaY() const { return y - startY; }
    float getDistance() const {
        float dx = getTotalDeltaX();
        float dy = getTotalDeltaY();
        return std::sqrt(dx * dx + dy * dy);
    }
};

// Touch event object
class LFTouchEvent {
public:
    LFTouchEventType type;
    LFEventPhase phase = LFEventPhase::None;

    // Touch point data
    std::vector<LFTouchPoint> touches;          // All touch points
    std::vector<LFTouchID> changedTouches;      // Changed touch point IDs

    // Event propagation control
    bool propagationStopped = false;            // Stop propagation
    bool defaultPrevented = false;              // Prevent default behavior

    // Target nodes
    std::weak_ptr<LFNode> target;               // Event target (HitTest result)
    std::weak_ptr<LFNode> currentTarget;        // Current processing node

    // Timestamp
    double timestamp = 0.0;

    // Get primary touch point (usually the first one)
    const LFTouchPoint* getPrimaryTouch() const {
        return touches.empty() ? nullptr : &touches[0];
    }

    // Get touch point by ID
    const LFTouchPoint* getTouchById(LFTouchID id) const {
        for (const auto& touch : touches) {
            if (touch.id == id) {
                return &touch;
            }
        }
        return nullptr;
    }

    // Get changed touch points
    std::vector<LFTouchPoint> getChangedTouchPoints() const {
        std::vector<LFTouchPoint> result;
        for (auto id : changedTouches) {
            for (auto& touch : touches) {
                if (touch.id == id) {
                    result.push_back(touch);
                    break;
                }
            }
        }
        return result;
    }

    // Stop propagation
    void stopPropagation() { propagationStopped = true; }
    void preventDefault() { defaultPrevented = true; }
};

class LFKeyEvent {
public:
    LFKeyEventType type = LFKeyEventType::Down;
    LFKeyCode keyCode = LFKeyCode::Unknown;
    uint32_t codepoint = 0;
    uint32_t modifiers = LFKeyModNone;
    bool repeat = false;
    bool propagationStopped = false;

    std::weak_ptr<LFNode> target;
    std::weak_ptr<LFNode> currentTarget;

    void stopPropagation() { propagationStopped = true; }
};

// Point structure
struct LFPoint {
    float x = 0.0f;
    float y = 0.0f;

    LFPoint() = default;
    LFPoint(float x, float y) : x(x), y(y) {}

    float distance(const LFPoint& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    LFPoint operator-(const LFPoint& other) const {
        return LFPoint(x - other.x, y - other.y);
    }

    LFPoint operator+(const LFPoint& other) const {
        return LFPoint(x + other.x, y + other.y);
    }

    LFPoint operator*(float scalar) const {
        return LFPoint(x * scalar, y * scalar);
    }

    LFPoint operator/(float scalar) const {
        return LFPoint(x / scalar, y / scalar);
    }
};

// Rectangle structure
struct LFRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    LFRect() = default;
    LFRect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}

    bool contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }

    bool contains(const LFPoint& p) const {
        return contains(p.x, p.y);
    }

    float getRight() const { return x + width; }
    float getBottom() const { return y + height; }
};

#endif // LEAF_LFEVENT_H
