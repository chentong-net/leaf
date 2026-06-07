#include "LFLocalTimePlatform.h"

#include <cstdlib>
#include <filesystem>
#include <unordered_map>

#if defined(_WIN32)
#include <windows.h>
#endif

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

#if defined(_WIN32)
std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return "";
    }

    const int utf8Count = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );
    if (utf8Count <= 0) {
        return "";
    }

    std::string utf8(static_cast<size_t>(utf8Count), '\0');
    const int converted = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        utf8.data(),
        utf8Count,
        nullptr,
        nullptr
    );
    if (converted <= 0) {
        return "";
    }

    return utf8;
}

std::string mapWindowsTimezoneToIana(const std::string& windowsZone) {
    static const std::unordered_map<std::string, std::string> kWindowsToIana = {
        {"Arab Standard Time", "Asia/Riyadh"},
        {"Arabian Standard Time", "Asia/Dubai"},
        {"Argentina Standard Time", "America/Buenos_Aires"},
        {"AUS Eastern Standard Time", "Australia/Sydney"},
        {"Cen. Australia Standard Time", "Australia/Adelaide"},
        {"Central Europe Standard Time", "Europe/Budapest"},
        {"Central European Standard Time", "Europe/Warsaw"},
        {"Central Standard Time", "America/Chicago"},
        {"China Standard Time", "Asia/Shanghai"},
        {"E. Australia Standard Time", "Australia/Brisbane"},
        {"E. South America Standard Time", "America/Sao_Paulo"},
        {"Eastern Standard Time", "America/New_York"},
        {"Egypt Standard Time", "Africa/Cairo"},
        {"FLE Standard Time", "Europe/Kyiv"},
        {"GMT Standard Time", "Europe/London"},
        {"Greenland Standard Time", "America/Godthab"},
        {"Greenwich Standard Time", "Atlantic/Reykjavik"},
        {"India Standard Time", "Asia/Kolkata"},
        {"Israel Standard Time", "Asia/Jerusalem"},
        {"Japan Standard Time", "Asia/Tokyo"},
        {"Korea Standard Time", "Asia/Seoul"},
        {"Mountain Standard Time", "America/Denver"},
        {"New Zealand Standard Time", "Pacific/Auckland"},
        {"Newfoundland Standard Time", "America/St_Johns"},
        {"Pacific SA Standard Time", "America/Santiago"},
        {"Pacific Standard Time", "America/Los_Angeles"},
        {"Romance Standard Time", "Europe/Paris"},
        {"Russian Standard Time", "Europe/Moscow"},
        {"SA Pacific Standard Time", "America/Bogota"},
        {"SE Asia Standard Time", "Asia/Bangkok"},
        {"Singapore Standard Time", "Asia/Singapore"},
        {"South Africa Standard Time", "Africa/Johannesburg"},
        {"Taipei Standard Time", "Asia/Taipei"},
        {"Tokyo Standard Time", "Asia/Tokyo"},
        {"Turkey Standard Time", "Europe/Istanbul"},
        {"US Mountain Standard Time", "America/Phoenix"},
        {"W. Australia Standard Time", "Australia/Perth"},
        {"W. Europe Standard Time", "Europe/Berlin"}
    };

    auto it = kWindowsToIana.find(windowsZone);
    if (it != kWindowsToIana.end()) {
        return it->second;
    }

    // TODO: Replace this curated Windows -> IANA list with a complete CLDR mapping.
    return windowsZone;
}
#endif

std::string resolvePosixTimezone() {
    const char* tzValue = std::getenv("TZ");
    if (tzValue && tzValue[0]) {
        return normalizeTimezoneValue(tzValue);
    }

    std::error_code ec;
    const std::filesystem::path localtimePath = std::filesystem::read_symlink("/etc/localtime", ec);
    if (!ec && !localtimePath.empty()) {
        const std::string normalized = normalizeTimezoneValue(localtimePath.string());
        if (!normalized.empty()) {
            return normalized;
        }
    }

#if defined(__APPLE__) || defined(__linux__)
    std::time_t now = std::time(nullptr);
    std::tm localTm{};
#if defined(_WIN32)
    if (localtime_s(&localTm, &now) == 0 && localTm.tm_zone) {
        return localTm.tm_zone;
    }
#else
    if (localtime_r(&now, &localTm) != nullptr && localTm.tm_zone) {
        return localTm.tm_zone;
    }
#endif
#endif

    return "";
}

} // namespace

void LFLocalTimePlatform::getTimezone(LFLocalTimePlatformTimezoneCallback callback) {
    if (!callback) {
        return;
    }

    LFLocalTimePlatformTimezoneResult result;

#if defined(_WIN32)
    DYNAMIC_TIME_ZONE_INFORMATION timeZoneInfo{};
    const DWORD status = GetDynamicTimeZoneInformation(&timeZoneInfo);
    if (status != TIME_ZONE_ID_INVALID) {
        std::wstring rawTimezone = timeZoneInfo.TimeZoneKeyName;
        if (rawTimezone.empty()) {
            rawTimezone = timeZoneInfo.StandardName;
        }

        result.timezone = mapWindowsTimezoneToIana(normalizeTimezoneValue(wideToUtf8(rawTimezone)));
        result.ok = !result.timezone.empty();
    }
#else
    result.timezone = resolvePosixTimezone();
    result.ok = !result.timezone.empty();
#endif

    if (!result.ok) {
        result.error = "timezone_unavailable";
    }

    callback(result);
}
