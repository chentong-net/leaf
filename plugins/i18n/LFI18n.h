#ifndef LEAF_LFI18N_H
#define LEAF_LFI18N_H

#include "LFI18nManager.h"

class LFI18n {
public:
    static void initialize(LFI18nInitCallback callback = nullptr);
    static void initializeFromAsset(const std::string& assetPath, LFI18nInitCallback callback = nullptr);

    static bool isReady();
    static bool setLanguage(const LFLocale& locale);
    static bool setLanguage(const std::string& languageTag);

    static LFLocale getCurrentLanguage();
    static LFLocale getDefaultLanguage();
    static LFLocale getSystemLanguage();

    static std::string tr(const std::string& key);
    static void bindText(const std::shared_ptr<LFText>& textNode, const std::string& key);
};

#endif // LEAF_LFI18N_H
