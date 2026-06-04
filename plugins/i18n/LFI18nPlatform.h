#ifndef LEAF_LFI18NPLATFORM_H
#define LEAF_LFI18NPLATFORM_H

#include "LFLocale.h"

struct LFI18nPlatformLocaleResult {
    bool ok = false;
    LFLocale locale;
    std::string error;
};

using LFI18nPlatformLocaleCallback = std::function<void(const LFI18nPlatformLocaleResult&)>;

class LFI18nPlatform {
public:
    static void getSystemLanguage(LFI18nPlatformLocaleCallback callback);
};

#endif // LEAF_LFI18NPLATFORM_H
