#include "layout/LFLayoutFactory.h"
#include "layout/yoga/LFYogaLayout.h"

std::unique_ptr<LFLayout> LFLayoutFactory::create() {
    return std::make_unique<LFYogaLayout>();
}
