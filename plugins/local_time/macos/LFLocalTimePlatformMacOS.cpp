#include "LFLocalTimePlatform.h"

#include <cstdlib>
#include <ctime>
#include <filesystem>

namespace {

std::string normalizeTimezoneValue(std::string value) {
    if (value.empty()) {
        return value;
    }

    if (value[0] == ':') {
        value.erase(0, 1);
    }

    const size_t commaPos = value.find(',');
    if (commaPos != std::string::npos) {
        value = value.substr(0, commaPos);
    }

    const std::string zoneinfoMarker = "zoneinfo/";
    const size_t zoneinfoPos = value.find(zoneinfoMarker);
    if (zoneinfoPos != std::string::npos) {
        value = value.substr(zoneinfoPos + zoneinfoMarker.size());
    }

    std::replace(value.begin(), value.end(), '\\', '/');
    return value;
}

std::string resolvePosixTimezone() {
    const char* tzValue = std::getenv("TZ");
    if (tzValue && tzValue[0]) {
        return normalizeTimezoneValue(tzValue);
    }

    std::error_code ec;
    const std::filesystem::path localtimePath =
            std::filesystem::read_symlink("/etc/localtime", ec);
    if (!ec && !localtimePath.empty()) {
        const std::string normalized =
                normalizeTimezoneValue(localtimePath.string());
        if (!normalized.empty()) {
            return normalized;
        }
    }

    std::time_t now = std::time(nullptr);
    std::tm localTm{};
    if (localtime_r(&now, &localTm) != nullptr && localTm.tm_zone) {
        return localTm.tm_zone;
    }

    return "";
}

} // namespace

void LFLocalTimePlatform::getTimezone(
        LFLocalTimePlatformTimezoneCallback callback) {
    if (!callback) {
        return;
    }

    LFLocalTimePlatformTimezoneResult result;
    result.timezone = resolvePosixTimezone();
    result.ok = !result.timezone.empty();
    if (!result.ok) {
        result.error = "timezone_unavailable";
    }

    callback(result);
}
