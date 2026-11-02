#ifndef LEAF_LFLAYOUTFACTORY_H
#define LEAF_LFLAYOUTFACTORY_H

#include <memory>

class LFLayout;

class LFLayoutFactory {
public:
    static std::unique_ptr<LFLayout> create();
};

#endif // LEAF_LFLAYOUTFACTORY_H
