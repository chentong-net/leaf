#ifndef LEAF_LFLOCALTIME_H
#define LEAF_LFLOCALTIME_H

#include "LFDef.h"

struct LFLocalTimeValue {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int millisecond = 0;
    int utcOffsetMinutes = 0;
    int64_t epochMillis = 0;
    std::string timezone;
};

class LFLocalTime {
public:
    static LFLocalTimeValue now();
    static int64_t nowMillis();
    static int utcOffsetMinutes();
    static std::string timezone();
};

#endif // LEAF_LFLOCALTIME_H
