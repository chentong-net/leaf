#include "LFLocalTimePlatform.h"

void LFLocalTimePlatform::getTimezone(LFLocalTimePlatformTimezoneCallback callback) {
    if (!callback) {
        return;
    }

    LFLocalTimePlatformTimezoneResult result;
    result.ok = false;
    result.error = "timezone_unsupported";
    callback(result);
}
