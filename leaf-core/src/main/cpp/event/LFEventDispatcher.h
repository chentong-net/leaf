//
// Created by Chen Tong on 2026/1/21.
// Event System - Event Dispatcher (Phase 1: Basic events, no gestures)
//

#ifndef LEAF_LFEVENTDISPATCHER_H
#define LEAF_LFEVENTDISPATCHER_H

#include "LFEvent.h"
#include "LFHitTest.h"
#include "../LFNode.h"
#include <map>
#include <vector>
#include <memory>

// Event dispatcher (singleton)
// Phase 1: Basic touch event dispatching with capture/target/bubble phases
// Phase 2+ will add gesture recognition
class LFEventDispatcher {
public:
    static LFEventDispatcher& getInstance();

    // Disable copy
    LFEventDispatcher(const LFEventDispatcher&) = delete;
    LFEventDispatcher& operator=(const LFEventDispatcher&) = delete;

    // Dispatch raw touch event
    void dispatchTouchEvent(
        LFTouchEventType type,
        const std::vector<LFTouchPoint>& touches,
        const std::vector<LFTouchID>& changedIDs,
        std::shared_ptr<LFNode> root
    );

private:
    LFEventDispatcher() = default;
    ~LFEventDispatcher() = default;

    // Event dispatch core flow
    void performDispatch(LFTouchEvent& event, std::shared_ptr<LFNode> root);

    // Three-phase dispatch
    void capturePhase(LFTouchEvent& event, const std::vector<std::shared_ptr<LFNode>>& path);
    void targetPhase(LFTouchEvent& event, std::shared_ptr<LFNode> target);
    void bubblePhase(LFTouchEvent& event, const std::vector<std::shared_ptr<LFNode>>& path);

    // Invoke node listener
    void invokeListener(
        std::shared_ptr<LFNode> node,
        LFTouchEvent& event
    );

    // Build event path from root to target
    std::vector<std::shared_ptr<LFNode>> buildEventPath(
        std::shared_ptr<LFNode> root,
        std::shared_ptr<LFNode> target
    );

    // Touch point state tracking
    std::map<LFTouchID, std::weak_ptr<LFNode>> m_touchTargets;  // Touch point → target node
    std::map<LFTouchID, LFTouchPoint> m_activeTouches;          // Active touch points
};

#endif // LEAF_LFEVENTDISPATCHER_H
