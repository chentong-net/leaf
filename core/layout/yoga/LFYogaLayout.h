#ifndef LEAF_LFYOGALAYOUT_H
#define LEAF_LFYOGALAYOUT_H

#include "layout/LFLayout.h"
#include "Yoga.h"

#include <memory>

class LFYogaLayout final : public LFLayout {
public:
    LFYogaLayout();
    ~LFYogaLayout() override;

    void setStyle(const LFLayoutStyle& style) override;
    void insertChild(LFLayout& child, size_t index) override;
    void removeChild(LFLayout& child) override;
    void setMeasureFunction(LFMeasureFunction function) override;
    void setBaselineFunction(LFBaselineFunction function) override;
    void markDirty() override;
    void calculate(const LFLayoutConstraints& constraints) override;
    void updateFrame() override;
    const LFLayoutFrame& getFrame() const override;

private:
    static YGSize measureCallback(YGNodeRef node,
                                  float width,
                                  YGMeasureMode widthMode,
                                  float height,
                                  YGMeasureMode heightMode);
    static float baselineCallback(YGNodeRef node, float width, float height);

    YGNodeRef m_node = nullptr;
    LFLayoutFrame m_frame;
    LFMeasureFunction m_measureFunction;
    LFBaselineFunction m_baselineFunction;
};

#endif // LEAF_LFYOGALAYOUT_H
