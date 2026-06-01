//
// Component lab demo app.
//

#ifndef COMPONENTLAB_H
#define COMPONENTLAB_H

#include "LFEngine.h"

class ComponentLab {
public:
    static std::shared_ptr<ComponentLab> create();

    LFNode::Ptr start();

    std::shared_ptr<LFNavigator> getNavigator() const { return m_navigator; }

private:
    std::shared_ptr<LFNavigator> m_navigator;
};

#endif // COMPONENTLAB_H
