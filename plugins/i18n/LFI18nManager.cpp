#include "LFI18nManager.h"

#include "LFJSONParser.h"
#include "LFPathProvider.h"
#include "plugin/LFPlugin.h"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace {

constexpr const char* kDefaultAssetPath = "i18n.json";
constexpr const char* kDefaultLanguageTag = "zh-CN";
constexpr const char* kCacheFileName = "i18n_cache.json";

bool localeLanguageMatches(const LFLocale& lhs, const LFLocale& rhs) {
    return !lhs.languageCode.empty() && lhs.languageCode == rhs.languageCode;
}

std::string readCachedLanguage(const std::string& cacheFilePath) {
    if (cacheFilePath.empty()) {
        return "";
    }

    std::ifstream input(std::filesystem::u8path(cacheFilePath), std::ios::binary);
    if (!input.is_open()) {
        return "";
    }

    const std::string content(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>()
    );
    if (content.empty()) {
        return "";
    }

    try {
        const auto json = LFJSONParser::parse(content);
        if (!json || !json->contains("language") || !json->at("language").isString()) {
            return "";
        }
        return LFLocale::fromTag(json->at("language").asString()).toTag();
    } catch (...) {
        return "";
    }
}

} // namespace

LFI18nManager& LFI18nManager::getInstance() {
    static LFI18nManager instance;
    return instance;
}

void LFI18nManager::initialize(LFI18nInitCallback callback) {
    initializeFromAsset(kDefaultAssetPath, std::move(callback));
}

void LFI18nManager::initializeFromAsset(const std::string& assetPath, LFI18nInitCallback callback) {
    const std::string resolvedAssetPath = assetPath.empty() ? kDefaultAssetPath : assetPath;

    bool shouldStart = false;
    bool alreadyReady = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (callback) {
            m_initCallbacks.push_back(std::move(callback));
        }

        if (m_ready && m_assetPath == resolvedAssetPath) {
            alreadyReady = true;
        } else if (!m_loading) {
            m_loading = true;
            m_assetPath = resolvedAssetPath;
            m_ready = false;
            shouldStart = true;
        }
    }

    if (alreadyReady) {
        notifyInitializationCallbacks(true);
    } else if (shouldStart) {
        startInitialization(resolvedAssetPath);
    }
}

bool LFI18nManager::isReady() {
    ensureInitializationStarted();
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ready;
}

bool LFI18nManager::setLanguage(const LFLocale& locale) {
    return setLanguage(locale.toTag());
}

bool LFI18nManager::setLanguage(const std::string& languageTag) {
    const std::string normalizedTag = LFLocale::fromTag(languageTag).toTag();
    if (normalizedTag.empty()) {
        return false;
    }

    ensureInitializationStarted();

    bool applied = false;
    bool isReady = false;
    std::string cacheLanguageTag = normalizedTag;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingLanguageTag = normalizedTag;
        isReady = m_ready;
        if (m_ready) {
            applied = applyCurrentLanguageLocked(normalizedTag);
            if (!m_currentLanguageTag.empty()) {
                cacheLanguageTag = m_currentLanguageTag;
            }
        }
    }

    if (isReady) {
        persistCachedLanguage(cacheLanguageTag);
    }
    return applied;
}

LFLocale LFI18nManager::getCurrentLanguage() {
    ensureInitializationStarted();

    std::lock_guard<std::mutex> lock(m_mutex);
    return LFLocale::fromTag(m_currentLanguageTag);
}

LFLocale LFI18nManager::getDefaultLanguage() {
    ensureInitializationStarted();

    std::lock_guard<std::mutex> lock(m_mutex);
    return LFLocale::fromTag(m_defaultLanguageTag);
}

LFLocale LFI18nManager::getSystemLanguage() {
    ensureInitializationStarted();

    std::lock_guard<std::mutex> lock(m_mutex);
    return LFLocale::fromTag(m_systemLanguageTag);
}

std::string LFI18nManager::get(const std::string& key) {
    ensureInitializationStarted();
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_ready) {
        return key;
    }
    return resolveTextLocked(key);
}

std::string LFI18nManager::tr(const std::string& key) {
    return get(key);
}

void LFI18nManager::ensureInitializationStarted() {
    initializeFromAsset(kDefaultAssetPath, nullptr);
}

void LFI18nManager::startInitialization(const std::string& resolvedAssetPath) {
    LFResourceProvider::getInstance().fetchAsset(
            resolvedAssetPath,
            [this](std::shared_ptr<LFData> data) {
                const ParsedTranslations parsed = parseTranslations(data);
                if (!parsed.ok) {
                    finishInitializationFailure();
                    return;
                }

                auto sharedParsed = std::make_shared<ParsedTranslations>(parsed);
                LFI18nPlatform::getSystemLanguage(
                        [this, sharedParsed](const LFI18nPlatformLocaleResult& systemLocale) {
                            LFPathProvider::getApplicationSupportPath(
                                    [this, sharedParsed, systemLocale](const LFPathProviderResult& pathResult) {
                                        std::string cacheFilePath;
                                        std::string cachedLanguageTag;
                                        if (pathResult.ok && !pathResult.path.empty()) {
                                            const auto path =
                                                    std::filesystem::u8path(pathResult.path) /
                                                    kCacheFileName;
                                            cacheFilePath = path.u8string();
                                            cachedLanguageTag = readCachedLanguage(cacheFilePath);
                                        }
                                        finishInitialization(
                                                *sharedParsed,
                                                systemLocale.ok ? systemLocale.locale.toTag() : "",
                                                cachedLanguageTag,
                                                cacheFilePath
                                        );
                                    }
                            );
                        }
                );
            }
    );
}

void LFI18nManager::finishInitialization(
        const ParsedTranslations& parsed,
        const std::string& systemLanguageTag,
        const std::string& cachedLanguageTag,
        const std::string& cacheFilePath) {
    bool shouldPersistPending = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_translations = parsed.translations;
        m_languageOrder = parsed.languageOrder;
        m_defaultLanguageTag = resolveDefaultLanguageTagLocked(
                parsed.defaultLanguageTag.empty() ? kDefaultLanguageTag : parsed.defaultLanguageTag
        );
        m_systemLanguageTag = systemLanguageTag;
        m_cacheFilePath = cacheFilePath;

        const std::string requestedTag = !m_pendingLanguageTag.empty()
                                         ? m_pendingLanguageTag
                                         : (!cachedLanguageTag.empty()
                                            ? cachedLanguageTag
                                            : m_systemLanguageTag);
        if (!applyCurrentLanguageLocked(requestedTag)) {
            m_currentLanguageTag = m_defaultLanguageTag;
        }

        m_ready = true;
        m_loading = false;
        shouldPersistPending = !m_pendingLanguageTag.empty() &&
                               !m_currentLanguageTag.empty();
    }

    if (shouldPersistPending) {
        LFLocale current;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            current = LFLocale::fromTag(m_currentLanguageTag);
        }
        persistCachedLanguage(current.toTag());
    }

    notifyInitializationCallbacks(true);
}

void LFI18nManager::finishInitializationFailure() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        clearStateLocked();
        m_loading = false;
    }
    notifyInitializationCallbacks(false);
}

void LFI18nManager::notifyInitializationCallbacks(bool ready) {
    std::vector<LFI18nInitCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        callbacks.swap(m_initCallbacks);
    }

    for (auto& callback : callbacks) {
        if (!callback) {
            continue;
        }
        LFPluginCenter::dispatchToMain([callback = std::move(callback), ready]() {
            callback(ready);
        });
    }
}

LFI18nManager::ParsedTranslations LFI18nManager::parseTranslations(const std::shared_ptr<LFData>& data) const {
    ParsedTranslations parsed;
    if (!data || !data->data || data->size == 0) {
        return parsed;
    }

    try {
        auto root = LFJSONParser::parse(data->data, data->size);
        if (!root || !root->contains("translations")) {
            return parsed;
        }

        if (root->contains("defaultLanguage")) {
            parsed.defaultLanguageTag = LFLocale::fromTag(root->at("defaultLanguage").asString()).toTag();
        }

        auto translationsValue = root->at("translations");
        if (!translationsValue.isObject()) {
            return parsed;
        }

        auto translationsObject = translationsValue.asObject();
        for (auto& item : translationsObject->raw()) {
            if (!item.second.isObject()) {
                continue;
            }

            const std::string languageTag = LFLocale::fromTag(item.first).toTag();
            if (languageTag.empty()) {
                continue;
            }

            std::unordered_map<std::string, std::string> keyValues;
            auto languageObject = item.second.asObject();
            for (auto& textItem : languageObject->raw()) {
                if (!textItem.second.isString()) {
                    continue;
                }
                keyValues[textItem.first] = textItem.second.asString();
            }

            if (!keyValues.empty()) {
                parsed.languageOrder.push_back(languageTag);
                parsed.translations[languageTag] = std::move(keyValues);
            }
        }

        parsed.ok = !parsed.translations.empty();
    } catch (...) {
        parsed.ok = false;
    }

    return parsed;
}

void LFI18nManager::clearStateLocked() {
    m_ready = false;
    m_defaultLanguageTag = kDefaultLanguageTag;
    m_currentLanguageTag.clear();
    m_systemLanguageTag.clear();
    m_translations.clear();
    m_languageOrder.clear();
}

bool LFI18nManager::applyCurrentLanguageLocked(const std::string& requestedTag) {
    const std::string resolvedTag = resolveAvailableLanguageTagLocked(requestedTag);
    if (resolvedTag.empty()) {
        if (!m_defaultLanguageTag.empty()) {
            m_currentLanguageTag = m_defaultLanguageTag;
        }
        return false;
    }

    m_currentLanguageTag = resolvedTag;
    return true;
}

std::string LFI18nManager::resolveAvailableLanguageTagLocked(const std::string& requestedTag) const {
    if (m_translations.empty()) {
        return "";
    }

    const std::string normalizedTag = LFLocale::fromTag(requestedTag).toTag();
    if (normalizedTag.empty()) {
        return "";
    }

    auto exactIt = m_translations.find(normalizedTag);
    if (exactIt != m_translations.end()) {
        return exactIt->first;
    }

    const LFLocale requestedLocale = LFLocale::fromTag(normalizedTag);
    std::string languageOnlyMatch;

    for (const auto& entry : m_translations) {
        const LFLocale candidate = LFLocale::fromTag(entry.first);
        if (!localeLanguageMatches(candidate, requestedLocale)) {
            continue;
        }
        if (!requestedLocale.scriptCode.empty() &&
            candidate.scriptCode == requestedLocale.scriptCode) {
            return entry.first;
        }
        if (!requestedLocale.countryCode.empty() &&
            candidate.countryCode == requestedLocale.countryCode) {
            return entry.first;
        }
        if (languageOnlyMatch.empty()) {
            languageOnlyMatch = entry.first;
        }
    }

    return languageOnlyMatch;
}

std::string LFI18nManager::resolveDefaultLanguageTagLocked(const std::string& requestedTag) const {
    const std::string resolved = resolveAvailableLanguageTagLocked(requestedTag);
    if (!resolved.empty()) {
        return resolved;
    }
    if (!m_languageOrder.empty()) {
        return m_languageOrder.front();
    }
    if (!m_translations.empty()) {
        return m_translations.begin()->first;
    }
    return LFLocale::fromTag(requestedTag).toTag();
}

std::string LFI18nManager::resolveTextLocked(const std::string& key) const {
    if (key.empty()) {
        return "";
    }

    auto currentIt = m_translations.find(m_currentLanguageTag);
    if (currentIt != m_translations.end()) {
        auto textIt = currentIt->second.find(key);
        if (textIt != currentIt->second.end()) {
            return textIt->second;
        }
    }

    auto defaultIt = m_translations.find(m_defaultLanguageTag);
    if (defaultIt != m_translations.end()) {
        auto textIt = defaultIt->second.find(key);
        if (textIt != defaultIt->second.end()) {
            return textIt->second;
        }
    }

    return key;
}

bool LFI18nManager::persistCachedLanguage(const std::string& languageTag) const {
    const std::string normalizedTag = LFLocale::fromTag(languageTag).toTag();
    if (normalizedTag.empty()) {
        return false;
    }

    std::string cacheFilePath;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        cacheFilePath = m_cacheFilePath;
    }
    if (cacheFilePath.empty()) {
        return false;
    }

    const std::filesystem::path path = std::filesystem::u8path(cacheFilePath);
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << "{\n  \"language\": \"" << normalizedTag << "\"\n}";
    return output.good();
}
