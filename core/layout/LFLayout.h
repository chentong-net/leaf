#ifndef LEAF_LFLAYOUT_H
#define LEAF_LFLAYOUT_H

#include "layout/LFLayoutFrame.h"
#include "layout/LFLayoutStyle.h"

#include <cstddef>
#include <functional>

enum class LFMeasureMode {
    Undefined,
    Exactly,
    AtMost
};

struct LFMeasureConstraint {
    float size = 0.0f;
    LFMeasureMode mode = LFMeasureMode::Undefined;
};

struct LFMeasureInput {
    LFMeasureConstraint width;
    LFMeasureConstraint height;
};

struct LFSize {
    float width = 0.0f;
    float height = 0.0f;
};

struct LFBaselineInput {
    float width = 0.0f;
    float height = 0.0f;
    LFEdgeInsets padding;
    LFEdgeInsets border;
};

struct LFLayoutConstraints {
    float width = 0.0f;
    float height = 0.0f;
};

using LFMeasureFunction = std::function<LFSize(const LFMeasureInput&)>;
using LFBaselineFunction = std::function<float(const LFBaselineInput&)>;

/**
 * 单节点的抽象布局接口。
 */
class LFLayout {
public:
    virtual ~LFLayout() = default;

    virtual void setStyle(const LFLayoutStyle& style) = 0;

    virtual void insertChild(LFLayout& child, size_t index) = 0;
    virtual void removeChild(LFLayout& child) = 0;

    virtual void setMeasureFunction(LFMeasureFunction function) = 0;
    virtual void setBaselineFunction(LFBaselineFunction function) = 0;

    virtual void markDirty() = 0;
    virtual void calculate(const LFLayoutConstraints& constraints) = 0;
    virtual void updateFrame() = 0;
    virtual const LFLayoutFrame& getFrame() const = 0;
};

#endif // LEAF_LFLAYOUT_H
