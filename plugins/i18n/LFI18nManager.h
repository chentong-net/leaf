#ifndef LEAF_LFI18NMANAGER_H
#define LEAF_LFI18NMANAGER_H

#include "LFLocale.h"
#include "LFI18nPlatform.h"
#include "LFResourceProvider.h"

#include <condition_variable>

using LFI18nInitCallback = std::function<void(bool)>;

class LFI18nManager {
public:
    static LFI18nManager& getInstance();

    void initialize(LFI18nInitCallback callback = nullptr);
    void initializeFromAsset(const std::string& assetPath, LFI18nInitCallback callback = nullptr);

    bool isReady();
    bool setLanguage(const LFLocale& locale);
    bool setLanguage(const std::string& languageTag);

    LFLocale getCurrentLanguage();
    LFLocale getDefaultLanguage();
    LFLocale getSystemLanguage();

    std::string get(const std::string& key);
    std::string tr(const std::string& key);

private:
    struct ParsedTranslations {
        bool ok = false;
        std::string defaultLanguageTag;
        std::map<std::string, std::unordered_map<std::string, std::string>> translations;
        std::vector<std::string> languageOrder;
    };

    LFI18nManager() = default;

    bool ensureInitialized();
    bool ensureInitializedFromAsset(const std::string& assetPath);
    ParsedTranslations parseTranslations(const std::shared_ptr<LFData>& data) const;
    void clearStateLocked();
    bool applyCurrentLanguageLocked(const std::string& requestedTag);
    std::string resolveAvailableLanguageTagLocked(const std::string& requestedTag) const;
    std::string resolveDefaultLanguageTagLocked(const std::string& requestedTag) const;
    std::string resolveTextLocked(const std::string& key) const;
    bool persistCachedLanguage(const std::string& languageTag) const;
    std::string loadCachedLanguage() const;

    mutable std::mutex m_mutex;
    std::condition_variable m_stateCv;
    bool m_loading = false;
    bool m_ready = false;
    std::string m_assetPath = "i18n.json";
    std::string m_defaultLanguageTag = "zh-CN";
    std::string m_currentLanguageTag = "zh-CN";
    std::string m_systemLanguageTag;
    std::string m_pendingLanguageTag;
    std::map<std::string, std::unordered_map<std::string, std::string>> m_translations;
    std::vector<std::string> m_languageOrder;
};

#endif // LEAF_LFI18NMANAGER_H
