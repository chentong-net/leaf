//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFENGINE_H
#define LEAF_LFENGINE_H

#include "LFRenderNode.h"

class LFEngine {
public:
    static LFEngine& getInstance() {
        static LFEngine instance;
        return instance;
    }

    // 根节点
    void setRootNode(LFRenderNode::Ptr root) { m_rootNode = root; }

    // 每帧调用
    void update(float width, float height) {
        if (m_rootNode) {
            // 布局
            m_rootNode->calculateLayout(width, height);
        }
    }

    // 每帧调用
    void render(NVGcontext* vg) {
        if (m_rootNode) {
            // 渲染
            m_rootNode->render(vg);
        }
    }

private:
    LFEngine() = default;
    LFRenderNode::Ptr m_rootNode = nullptr;
};

#endif
