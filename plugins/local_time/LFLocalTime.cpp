#include "LFLocalTime.h"

#include "LFLocalTimePlatform.h"

#include <chrono>
#include <condition_variable>
#include <ctime>

namespace {

constexpr auto kTimezoneWaitTimeout = std::chrono::seconds(2);

std::mutex g_timezoneMutex;
bool g_timezoneResolved = false;
std::string g_cachedTimezone;

bool toLocalTm(std::time_t epochSeconds, std::tm* outTm) {
    if (!outTm) {
        return false;
    }

#if defined(_WIN32)
    return localtime_s(outTm, &epochSeconds) == 0;
#else
    return localtime_r(&epochSeconds, outTm) != nullptr;
#endif
}

std::time_t timegmPortable(std::tm* timeInfo) {
    if (!timeInfo) {
        return static_cast<std::time_t>(-1);
    }

#if defined(_WIN32)
    return _mkgmtime(timeInfo);
#else
    return timegm(timeInfo);
#endif
}

int resolveUtcOffsetMinutes(std::time_t epochSeconds, const std::tm& localTm) {
    std::tm localCopy = localTm;
    const std::time_t localAsUtc = timegmPortable(&localCopy);
    if (localAsUtc == static_cast<std::time_t>(-1)) {
        return 0;
    }

    return static_cast<int>(std::difftime(localAsUtc, epochSeconds) / 60.0);
}

std::string loadTimezoneSynchronously() {
    std::mutex waitMutex;
    std::condition_variable waitCv;
    bool completed = false;
    LFLocalTimePlatformTimezoneResult result;

    LFLocalTimePlatform::getTimezone([&](const LFLocalTimePlatformTimezoneResult& platformResult) {
        {
            std::lock_guard<std::mutex> lock(waitMutex);
            result = platformResult;
            completed = true;
        }
        waitCv.notify_one();
    });

    std::unique_lock<std::mutex> waitLock(waitMutex);
    if (!waitCv.wait_for(waitLock, kTimezoneWaitTimeout, [&completed]() {
        return completed;
    })) {
        return "";
    }

    return result.ok ? result.timezone : "";
}

} // namespace

LFLocalTimeValue LFLocalTime::now() {
    const auto nowPoint = std::chrono::system_clock::now();
    const int64_t epochMillis = std::chrono::duration_cast<std::chrono::milliseconds>(
        nowPoint.time_since_epoch()
    ).count();
    const std::time_t epochSeconds = std::chrono::system_clock::to_time_t(nowPoint);

    std::tm localTm{};
    if (!toLocalTm(epochSeconds, &localTm)) {
        LFLocalTimeValue value;
        value.epochMillis = epochMillis;
        value.timezone = timezone();
        return value;
    }

    LFLocalTimeValue value;
    value.year = localTm.tm_year + 1900;
    value.month = localTm.tm_mon + 1;
    value.day = localTm.tm_mday;
    value.hour = localTm.tm_hour;
    value.minute = localTm.tm_min;
    value.second = localTm.tm_sec;
    value.millisecond = static_cast<int>(epochMillis % 1000);
    value.utcOffsetMinutes = resolveUtcOffsetMinutes(epochSeconds, localTm);
    value.epochMillis = epochMillis;
    value.timezone = timezone();
    return value;
}

int64_t LFLocalTime::nowMillis() {
    const auto nowPoint = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        nowPoint.time_since_epoch()
    ).count();
}

int LFLocalTime::utcOffsetMinutes() {
    const std::time_t epochSeconds = std::time(nullptr);
    std::tm localTm{};
    if (!toLocalTm(epochSeconds, &localTm)) {
        return 0;
    }
    return resolveUtcOffsetMinutes(epochSeconds, localTm);
}

std::string LFLocalTime::timezone() {
    {
        std::lock_guard<std::mutex> lock(g_timezoneMutex);
        if (g_timezoneResolved) {
            return g_cachedTimezone;
        }
    }

    const std::string resolvedTimezone = loadTimezoneSynchronously();

    {
        std::lock_guard<std::mutex> lock(g_timezoneMutex);
        if (!g_timezoneResolved) {
            g_cachedTimezone = resolvedTimezone;
            g_timezoneResolved = true;
        }
        return g_cachedTimezone;
    }
}
