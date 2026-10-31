#include "LFLocalTime.h"

#include "LFLocalTimePlatform.h"

#include <chrono>
#include <cstdlib>
#include <ctime>

namespace {

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

std::string resolveHostTimezone() {
#if defined(__ANDROID__) || defined(__APPLE__) || defined(__linux__) || \
    defined(__OHOS__) || defined(__WEB__)
    const char* timezoneEnvironment = std::getenv("TZ");
    if (timezoneEnvironment && timezoneEnvironment[0]) {
        return timezoneEnvironment[0] == ':'
               ? std::string(timezoneEnvironment + 1)
               : std::string(timezoneEnvironment);
    }

    const std::time_t now = std::time(nullptr);
    std::tm localTime{};
    if (localtime_r(&now, &localTime) != nullptr && localTime.tm_zone) {
        return localTime.tm_zone;
    }
    return "";
#else
    return "";
#endif
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

    std::string resolvedTimezone = resolveHostTimezone();
#if defined(_WIN32)
    if (resolvedTimezone.empty()) {
        LFLocalTimePlatformTimezoneResult platformResult;
        LFLocalTimePlatform::getTimezone(
                [&platformResult](const LFLocalTimePlatformTimezoneResult& result) {
                    platformResult = result;
                }
        );
        if (platformResult.ok) {
            resolvedTimezone = platformResult.timezone;
        }
    }
#endif

    {
        std::lock_guard<std::mutex> lock(g_timezoneMutex);
        if (!g_timezoneResolved && !resolvedTimezone.empty()) {
            g_cachedTimezone = resolvedTimezone;
            g_timezoneResolved = true;
        }
        return resolvedTimezone.empty() ? g_cachedTimezone : resolvedTimezone;
    }
}
