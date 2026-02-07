//
// Created by Chen Tong on 2026/1/21.
// Event System - HitTest Engine
//

#ifndef LEAF_LFHITTEST_H
#define LEAF_LFHITTEST_H

#include "view/base/LFNode.h"
#include "LFEvent.h"
#include <vector>

// HitTest result
struct LFHitTestResult {
    std::vector<std::weak_ptr<LFNode>> path;  // Path from root to target
    LFPoint localPoint;                        // Local coordinate in target node

    bool hasTarget() const { return !path.empty(); }

    std::shared_ptr<LFNode> getTarget() const {
        if (path.empty()) return nullptr;
        return path.back().lock();
    }

    std::shared_ptr<LFNode> getRoot() const {
        if (path.empty()) return nullptr;
        return path.front().lock();
    }
};

// HitTest engine
class LFHitTestEngine {
public:
    // Perform HitTest
    static LFHitTestResult hitTest(
        std::shared_ptr<LFNode> root,
        float x, float y
    );

private:
    // Recursive HitTest implementation
    static bool hitTestRecursive(
        std::shared_ptr<LFNode> node,
        float x, float y,
        LFHitTestResult& result
    );

    // Inverse transform point
    static bool inverseTransformPoint(
        LFNode* node,
        float& x, float& y
    );

    // Check if point is in node bounds
    static bool isPointInBounds(
        LFNode* node,
        float x, float y
    );

    // Check if point is in rounded rect
    static bool isPointInRoundedRect(
        float x, float y,
        float width, float height,
        float radius
    );
};

#endif // LEAF_LFHITTEST_H
