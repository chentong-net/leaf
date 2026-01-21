//
// Created by Chen Tong on 2026/1/21.
// Gesture System - Gesture Arena Implementation
//

#include "LFGestureArena.h"
#include <algorithm>

// ==========================================
// LFGestureArenaEntry Implementation
// ==========================================

void LFGestureArenaEntry::resolve(LFGestureDisposition disposition) {
    // Entry resolved, trigger sweep
    // The actual acceptance/rejection will be handled by arena manager
    // Store the disposition for later use
    m_disposition = disposition;
}

// ==========================================
// LFGestureArenaManager Implementation
// ==========================================

LFGestureArenaManager& LFGestureArenaManager::getInstance() {
    static LFGestureArenaManager instance;
    return instance;
}

std::shared_ptr<LFGestureArenaEntry> LFGestureArenaManager::add(
    int pointer,
    std::shared_ptr<LFGestureArenaMember> member
) {
    if (!member) return nullptr;

    // Get or create arena state for this pointer
    auto& state = m_arenas[pointer];

    // Create entry
    auto entry = std::make_shared<LFGestureArenaEntry>(member);

    // Add to arena if still open
    if (state.isOpen) {
        state.members.push_back(entry);
    }

    return entry;
}

void LFGestureArenaManager::close(int pointer) {
    auto it = m_arenas.find(pointer);
    if (it == m_arenas.end()) return;

    auto& state = it->second;
    state.isOpen = false;

    // Try to resolve now that arena is closed
    tryToResolveArena(pointer, state);
}

void LFGestureArenaManager::sweep(int pointer) {
    auto it = m_arenas.find(pointer);
    if (it == m_arenas.end()) return;

    auto& state = it->second;

    // If held, don't sweep yet
    if (state.isHeld) return;

    tryToResolveArena(pointer, state);
}

void LFGestureArenaManager::hold(int pointer) {
    auto it = m_arenas.find(pointer);
    if (it != m_arenas.end()) {
        it->second.isHeld = true;
    }
}

void LFGestureArenaManager::release(int pointer) {
    auto it = m_arenas.find(pointer);
    if (it == m_arenas.end()) return;

    auto& state = it->second;
    state.isHeld = false;

    // Try to resolve now that hold is released
    tryToResolveArena(pointer, state);
}

void LFGestureArenaManager::clearAll() {
    m_arenas.clear();
}

void LFGestureArenaManager::tryToResolveArena(int pointer, LFArenaState& state) {
    // Don't resolve if held
    if (state.isHeld) return;

    // Don't resolve if still open (waiting for more members)
    if (state.isOpen) return;

    // Filter out dead members (weak_ptr expired)
    std::vector<std::shared_ptr<LFGestureArenaEntry>> aliveMembers;
    for (auto& entry : state.members) {
        if (!entry->getMember().expired()) {
            aliveMembers.push_back(entry);
        }
    }

    // No alive members, clean up and return
    if (aliveMembers.empty()) {
        m_arenas.erase(pointer);
        return;
    }

    // Only one member, they win by default
    if (aliveMembers.size() == 1) {
        auto winner = aliveMembers[0]->getMember().lock();
        if (winner) {
            winner->acceptGesture(pointer);
        }
        m_arenas.erase(pointer);
        return;
    }

    // Multiple members: find if any have accepted
    int acceptedIndex = -1;
    for (size_t i = 0; i < aliveMembers.size(); i++) {
        if (aliveMembers[i]->getDisposition() == LFGestureDisposition::Accepted) {
            acceptedIndex = (int)i;
            break;  // First to accept wins
        }
    }

    // If someone accepted, they win
    if (acceptedIndex >= 0) {
        auto winner = aliveMembers[acceptedIndex]->getMember().lock();

        if (winner) {
            // Accept the winner
            winner->acceptGesture(pointer);

            // Reject all others
            for (size_t i = 0; i < aliveMembers.size(); i++) {
                if ((int)i != acceptedIndex) {
                    auto loser = aliveMembers[i]->getMember().lock();
                    if (loser) {
                        loser->rejectGesture(pointer);
                    }
                }
            }

            // Clean up arena
            m_arenas.erase(pointer);
        }
    }

    // Note: If no one has accepted yet, keep arena alive
    // This handles cases where gestures need more input to decide
}

