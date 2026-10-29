#include "LFI18nPlatform.h"

#include <windows.h>

namespace {

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

} // namespace

void LFI18nPlatform::getSystemLanguage(LFI18nPlatformLocaleCallback callback) {
    if (!callback) {
        return;
    }

    LFI18nPlatformLocaleResult result;
    wchar_t buffer[LOCALE_NAME_MAX_LENGTH] = {0};
    const int length = GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH);
    if (length > 0) {
        result.locale = LFLocale::fromTag(wideToUtf8(buffer));
        result.ok = !result.locale.isEmpty();
    }

    if (!result.ok) {
        result.error = "system_language_unavailable";
    }

    callback(result);
}
