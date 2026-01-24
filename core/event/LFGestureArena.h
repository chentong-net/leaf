//
// Created by Chen Tong on 2026/1/21.
// Gesture System - Gesture Arena (Phase 3)
// 手势竞技场：处理多个手势识别器竞争同一事件流
//

#ifndef LEAF_LFGESTUREARENA_H
#define LEAF_LFGESTUREARENA_H

#include <memory>
#include <vector>
#include <map>
#include <functional>

// Forward declaration
class LFGestureRecognizer;

// Gesture arena entry disposition (手势识别器的决定)
enum class LFGestureDisposition {
    Accepted,   // 接受此手势
    Rejected    // 拒绝此手势
};

// Gesture arena member (竞技场成员)
class LFGestureArenaMember {
public:
    virtual ~LFGestureArenaMember() = default;

    // Called when this member wins the arena
    virtual void acceptGesture(int pointer) = 0;

    // Called when this member loses the arena
    virtual void rejectGesture(int pointer) = 0;
};

// Gesture arena entry (竞技场条目)
class LFGestureArenaEntry {
public:
    using Ptr = std::shared_ptr<LFGestureArenaEntry>;

    explicit LFGestureArenaEntry(std::weak_ptr<LFGestureArenaMember> member)
        : m_member(member), m_disposition(LFGestureDisposition::Rejected) {}

    // Resolve this entry's disposition
    void resolve(LFGestureDisposition disposition);

    std::weak_ptr<LFGestureArenaMember> getMember() const { return m_member; }
    LFGestureDisposition getDisposition() const { return m_disposition; }

private:
    std::weak_ptr<LFGestureArenaMember> m_member;
    LFGestureDisposition m_disposition;
};

// Gesture arena state
struct LFArenaState {
    std::vector<std::shared_ptr<LFGestureArenaEntry>> members;
    bool isOpen = true;         // Arena is open (can add new members)
    bool isHeld = false;        // Arena is held (delay sweep)
};

// Gesture arena manager (singleton)
// 管理所有触摸点的手势竞技场
class LFGestureArenaManager {
public:
    static LFGestureArenaManager& getInstance();

    // Disable copy
    LFGestureArenaManager(const LFGestureArenaManager&) = delete;
    LFGestureArenaManager& operator=(const LFGestureArenaManager&) = delete;

    // Add a member to the arena for a specific pointer
    std::shared_ptr<LFGestureArenaEntry> add(
        int pointer,
        std::shared_ptr<LFGestureArenaMember> member
    );

    // Close the arena for a pointer (no more members can be added)
    // This happens when the pointer goes up
    void close(int pointer);

    // Sweep the arena to resolve competition
    // Called when a member resolves (accept/reject)
    void sweep(int pointer);

    // Hold the arena (prevent sweep until release)
    void hold(int pointer);

    // Release held arena
    void release(int pointer);

    // Clear all arenas (for testing/reset)
    void clearAll();

private:
    LFGestureArenaManager() = default;
    ~LFGestureArenaManager() = default;

    // Try to resolve the arena if possible
    void tryToResolveArena(int pointer, LFArenaState& state);

    // Arena state for each pointer
    std::map<int, LFArenaState> m_arenas;
};

#endif // LEAF_LFGESTUREARENA_H
