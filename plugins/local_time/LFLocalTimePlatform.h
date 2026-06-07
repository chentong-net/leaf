#ifndef LEAF_LFLOCALTIMEPLATFORM_H
#define LEAF_LFLOCALTIMEPLATFORM_H

#include "LFDef.h"

struct LFLocalTimePlatformTimezoneResult {
    bool ok = false;
    std::string timezone;
    std::string error;
};

using LFLocalTimePlatformTimezoneCallback = std::function<void(const LFLocalTimePlatformTimezoneResult&)>;

class LFLocalTimePlatform {
public:
    static void getTimezone(LFLocalTimePlatformTimezoneCallback callback);
};

#endif // LEAF_LFLOCALTIMEPLATFORM_H
