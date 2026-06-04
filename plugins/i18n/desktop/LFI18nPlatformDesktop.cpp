#include "LFI18nPlatform.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#include <cstdlib>

namespace {

std::string normalizeLocaleTag(const std::string& raw) {
    std::string value = raw;
    const size_t dotPos = value.find('.');
    if (dotPos != std::string::npos) {
        value = value.substr(0, dotPos);
    }
    const size_t modifierPos = value.find('@');
    if (modifierPos != std::string::npos) {
        value = value.substr(0, modifierPos);
    }
    return LFLocale::fromTag(value).toTag();
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
#endif

} // namespace

void LFI18nPlatform::getSystemLanguage(LFI18nPlatformLocaleCallback callback) {
    if (!callback) {
        return;
    }

    LFI18nPlatformLocaleResult result;

#if defined(_WIN32)
    wchar_t buffer[LOCALE_NAME_MAX_LENGTH] = {0};
    const int length = GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH);
    if (length > 0) {
        result.locale = LFLocale::fromTag(wideToUtf8(buffer));
        result.ok = !result.locale.isEmpty();
    }
#else
    const char* candidates[] = {
        std::getenv("LC_ALL"),
        std::getenv("LC_MESSAGES"),
        std::getenv("LANG")
    };

    for (const char* candidate : candidates) {
        if (!candidate || !candidate[0]) {
            continue;
        }
        result.locale = LFLocale::fromTag(normalizeLocaleTag(candidate));
        if (!result.locale.isEmpty()) {
            result.ok = true;
            break;
        }
    }
#endif

    if (!result.ok) {
        result.error = "system_language_unavailable";
    }

    callback(result);
}
