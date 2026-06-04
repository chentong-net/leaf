#ifndef LEAF_LFI18NMANAGER_H
#define LEAF_LFI18NMANAGER_H

#include "LFLocale.h"
#include "LFI18nPlatform.h"
#include "LFResourceProvider.h"
#include "view/base/LFText.h"

using LFI18nInitCallback = std::function<void(bool)>;

class LFI18nManager {
public:
    static LFI18nManager& getInstance();

    void initialize(LFI18nInitCallback callback = nullptr);
    void initializeFromAsset(const std::string& assetPath, LFI18nInitCallback callback = nullptr);

    bool isReady() const;
    bool setLanguage(const LFLocale& locale);
    bool setLanguage(const std::string& languageTag);

    LFLocale getCurrentLanguage() const;
    LFLocale getDefaultLanguage() const;
    LFLocale getSystemLanguage() const;

    std::string tr(const std::string& key) const;
    void bindText(const std::shared_ptr<LFText>& textNode, const std::string& key);

private:
    struct TextBinding {
        std::weak_ptr<LFText> textNode;
        std::string key;
    };

    struct ParsedTranslations {
        bool ok = false;
        std::string defaultLanguageTag;
        std::map<std::string, std::unordered_map<std::string, std::string>> translations;
        std::vector<std::string> languageOrder;
    };

    LFI18nManager() = default;

    ParsedTranslations parseTranslations(const std::shared_ptr<LFData>& data) const;
    void finalizeInitialize(
            ParsedTranslations parsed,
            LFI18nPlatformLocaleResult platformLocale,
            std::vector<LFI18nInitCallback> callbacks);
    bool applyCurrentLanguage(const std::string& requestedTag);
    std::string resolveAvailableLanguageTagLocked(const std::string& requestedTag) const;
    std::string resolveDefaultLanguageTagLocked(const std::string& requestedTag) const;
    std::string resolveTextLocked(const std::string& key) const;
    void refreshBindings();

    mutable std::mutex m_mutex;
    bool m_loading = false;
    bool m_ready = false;
    std::string m_assetPath = "i18n.json";
    std::string m_defaultLanguageTag = "zh-CN";
    std::string m_currentLanguageTag = "zh-CN";
    std::string m_systemLanguageTag;
    std::string m_pendingLanguageTag;
    std::map<std::string, std::unordered_map<std::string, std::string>> m_translations;
    std::vector<std::string> m_languageOrder;
    std::vector<TextBinding> m_bindings;
    std::vector<LFI18nInitCallback> m_pendingCallbacks;
};

#endif // LEAF_LFI18NMANAGER_H
