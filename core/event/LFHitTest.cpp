//
// Created by Chen Tong on 2026/1/21.
// Event System - HitTest Engine Implementation
//

#include "LFHitTest.h"
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;

}

LFHitTestResult LFHitTestEngine::hitTest(
    std::shared_ptr<LFNode> root,
    float x, float y
) {
    LFHitTestResult result;

    if (!root) return result;

    // Start recursive HitTest from root node
    hitTestRecursive(root, x, y, result);

    return result;
}

bool LFHitTestEngine::hitTestRecursive(
    std::shared_ptr<LFNode> node,
    float x, float y,
    LFHitTestResult& result
) {
    // 1. Basic filtering
    if (!node || !node->isVisible() || node->getOpacity() <= 0.0f) {
        return false;
    }

    // Skip nodes that don't participate in HitTest
    if (!node->isHitTestEnabled()) {
        return false;
    }

    // 2. Transform coordinates to node's local space
    float localX = x;
    float localY = y;

    // 减去父布局坐标系中的节点偏移。
    localX -= node->getLayoutX();
    localY -= node->getLayoutY();

    // Apply inverse transform
    if (!inverseTransformPoint(node.get(), localX, localY)) {
        return false;  // Transform not invertible (e.g., scale = 0)
    }

    // 3. Bounds check. Overflow-visible children remain hittable outside their
    // parent bounds; masksToBounds limits traversal to the visible region.
    float width = node->getLayoutWidth();
    float height = node->getLayoutHeight();
    const bool insideBounds = isPointInBounds(node.get(), localX, localY);
    if (!insideBounds && node->isMasksToBounds()) {
        return false;
    }

    // 4. Rounded rect check (if has border radius)
    const float radius = node->getRadius();
    if (insideBounds && radius > 0) {
        if (!isPointInRoundedRect(localX, localY, width, height, radius)) {
            if (node->isMasksToBounds()) {
                return false;
            }
        }
    }

    // 5. Reverse iterate children (last added is on top)
    const auto& children = node->getChildren();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if (hitTestRecursive(*it, localX, localY, result)) {
            // Child hit, add current node to path
            result.path.insert(result.path.begin(), node);
            return true;
        }
    }

    // 6. No child hit, check current node
    // Only nodes with touch enabled count as hit
    if (!insideBounds || !node->isTouchEnabled()) {
        return false;
    }

    result.path.push_back(node);
    result.localPoint = LFPoint(localX, localY);
    return true;
}

bool LFHitTestEngine::inverseTransformPoint(
    LFNode* node,
    float& x, float& y
) {
    const LFTransform& t = node->getTransform();

    float width = node->getLayoutWidth();
    float height = node->getLayoutHeight();
    float cx = width * 0.5f;
    float cy = height * 0.5f;

    // Inverse transform order (reverse of rendering):
    // Rendering: Translate → Rotate → Scale
    // Inverse: InvScale → InvRotate → InvTranslate

    // 1. Handle translate
    float tx = t.translateX + width * (t.translatePercentX / 100.0f);
    float ty = t.translateY + height * (t.translatePercentY / 100.0f);
    x -= tx;
    y -= ty;

    // 2. Handle rotate and scale (around center)
    if (t.rotate != 0 || t.scaleX != 1.0f || t.scaleY != 1.0f) {
        // Move to center
        x -= cx;
        y -= cy;

        if (std::abs(t.scaleX) < 0.001f || std::abs(t.scaleY) < 0.001f) {
            return false;  // Scale is 0, transform not invertible
        }

        // Rendering composes rotate * scale, so the inverse must apply
        // inverse rotate before inverse scale.
        if (t.rotate != 0) {
            float rad = -t.rotate * kPi / 180.0f;  // Reverse rotation
            float cosA = std::cos(rad);
            float sinA = std::sin(rad);
            float nx = x * cosA - y * sinA;
            float ny = x * sinA + y * cosA;
            x = nx;
            y = ny;
        }
        x /= t.scaleX;
        y /= t.scaleY;

        // Move back to origin
        x += cx;
        y += cy;
    }

    return true;
}

bool LFHitTestEngine::isPointInBounds(
    LFNode* node,
    float x, float y
) {
    float width = node->getLayoutWidth();
    float height = node->getLayoutHeight();
    return x >= 0 && x <= width && y >= 0 && y <= height;
}

bool LFHitTestEngine::isPointInRoundedRect(
    float x, float y,
    float width, float height,
    float radius
) {
    // Limit radius to half of minimum dimension
    radius = std::min(radius, std::min(width, height) * 0.5f);

    // Determine which region the point is in
    bool inLeft = x < radius;
    bool inRight = x > width - radius;
    bool inTop = y < radius;
    bool inBottom = y > height - radius;

    // Check four corner circular regions
    if (inLeft && inTop) {
        // Top-left corner
        float dx = x - radius;
        float dy = y - radius;
        return dx * dx + dy * dy <= radius * radius;
    } else if (inRight && inTop) {
        // Top-right corner
        float dx = x - (width - radius);
        float dy = y - radius;
        return dx * dx + dy * dy <= radius * radius;
    } else if (inLeft && inBottom) {
        // Bottom-left corner
        float dx = x - radius;
        float dy = y - (height - radius);
        return dx * dx + dy * dy <= radius * radius;
    } else if (inRight && inBottom) {
        // Bottom-right corner
        float dx = x - (width - radius);
        float dy = y - (height - radius);
        return dx * dx + dy * dy <= radius * radius;
    }

    // Middle region is always inside
    return true;
}
