#include "LFI18nPlatform.h"

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

} // namespace

void LFI18nPlatform::getSystemLanguage(LFI18nPlatformLocaleCallback callback) {
    if (!callback) {
        return;
    }

    LFI18nPlatformLocaleResult result;
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

    if (!result.ok) {
        result.error = "system_language_unavailable";
    }

    callback(result);
}
