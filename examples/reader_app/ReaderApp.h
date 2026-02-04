//
// Created by Chen Tong on 2026/2/2.
//

#ifndef APP_H
#define APP_H

#include <memory>
#include "component/LFNode.h"
#include "component/LFNavigator.h"

class ReaderApp {
public:
    static std::shared_ptr<ReaderApp> create();

    /**
     * 启动应用，构建 UI 树
     * @return 返回根节点 (Navigator)
     */
    LFNode::Ptr start();

    std::shared_ptr<LFNavigator> getNavigator() const { return m_navigator; }

private:
    std::shared_ptr<LFNavigator> m_navigator;
};

#endif // APP_H
