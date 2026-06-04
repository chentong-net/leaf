#include "LFI18nManager.h"

#include "LFJSONParser.h"
#include "LFPathProvider.h"

#include <chrono>
#include <filesystem>
#include <iterator>

namespace {

constexpr auto kSyncWaitTimeout = std::chrono::seconds(2);
constexpr const char* kDefaultAssetPath = "i18n.json";
constexpr const char* kDefaultLanguageTag = "zh-CN";
constexpr const char* kCacheFileName = "i18n_cache.json";

bool localeLanguageMatches(const LFLocale& lhs, const LFLocale& rhs) {
    return !lhs.languageCode.empty() && lhs.languageCode == rhs.languageCode;
}

std::shared_ptr<LFData> loadAssetSynchronously(const std::string& assetPath) {
    std::mutex waitMutex;
    std::condition_variable waitCv;
    bool completed = false;
    std::shared_ptr<LFData> loadedData;

    LFResourceProvider::getInstance().fetchAsset(assetPath, [&](std::shared_ptr<LFData> data) {
        {
            std::lock_guard<std::mutex> lock(waitMutex);
            loadedData = std::move(data);
            completed = true;
        }
        waitCv.notify_one();
    });

    std::unique_lock<std::mutex> waitLock(waitMutex);
    if (!waitCv.wait_for(waitLock, kSyncWaitTimeout, [&completed]() {
        return completed;
    })) {
        return nullptr;
    }
    return loadedData;
}

LFI18nPlatformLocaleResult loadSystemLanguageSynchronously() {
    std::mutex waitMutex;
    std::condition_variable waitCv;
    bool completed = false;
    LFI18nPlatformLocaleResult result;

    LFI18nPlatform::getSystemLanguage([&](const LFI18nPlatformLocaleResult& platformLocale) {
        {
            std::lock_guard<std::mutex> lock(waitMutex);
            result = platformLocale;
            completed = true;
        }
        waitCv.notify_one();
    });

    std::unique_lock<std::mutex> waitLock(waitMutex);
    if (!waitCv.wait_for(waitLock, kSyncWaitTimeout, [&completed]() {
        return completed;
    })) {
        result.ok = false;
        result.error = "get_system_language_timeout";
    }
    return result;
}

std::string loadApplicationSupportPathSynchronously() {
    std::mutex waitMutex;
    std::condition_variable waitCv;
    bool completed = false;
    LFPathProviderResult pathResult;

    LFPathProvider::getApplicationSupportPath([&](const LFPathProviderResult& result) {
        {
            std::lock_guard<std::mutex> lock(waitMutex);
            pathResult = result;
            completed = true;
        }
        waitCv.notify_one();
    });

    std::unique_lock<std::mutex> waitLock(waitMutex);
    if (!waitCv.wait_for(waitLock, kSyncWaitTimeout, [&completed]() {
        return completed;
    })) {
        return "";
    }
    if (!pathResult.ok || pathResult.path.empty()) {
        return "";
    }
    return pathResult.path;
}

std::filesystem::path resolveCacheFilePath() {
    const std::string appSupportPath = loadApplicationSupportPathSynchronously();
    if (appSupportPath.empty()) {
        return {};
    }
    return std::filesystem::u8path(appSupportPath) / kCacheFileName;
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
    const bool ready = ensureInitializedFromAsset(assetPath);
    if (callback) {
        callback(ready);
    }
}

bool LFI18nManager::isReady() {
    if (!ensureInitialized()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_ready;
    }

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

    ensureInitialized();

    bool applied = false;
    bool persistOnly = false;
    std::string cacheLanguageTag = normalizedTag;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingLanguageTag = normalizedTag;
        if (!m_ready) {
            persistOnly = true;
        } else {
            applied = applyCurrentLanguageLocked(normalizedTag);
            if (!m_currentLanguageTag.empty()) {
                cacheLanguageTag = m_currentLanguageTag;
            }
        }
    }

    const bool persisted = persistCachedLanguage(cacheLanguageTag);
    if (persistOnly) {
        return persisted;
    }
    return applied;
}

LFLocale LFI18nManager::getCurrentLanguage() {
    ensureInitialized();

    std::lock_guard<std::mutex> lock(m_mutex);
    return LFLocale::fromTag(m_currentLanguageTag);
}

LFLocale LFI18nManager::getDefaultLanguage() {
    ensureInitialized();

    std::lock_guard<std::mutex> lock(m_mutex);
    return LFLocale::fromTag(m_defaultLanguageTag);
}

LFLocale LFI18nManager::getSystemLanguage() {
    ensureInitialized();

    std::lock_guard<std::mutex> lock(m_mutex);
    return LFLocale::fromTag(m_systemLanguageTag);
}

std::string LFI18nManager::get(const std::string& key) {
    if (!ensureInitialized()) {
        return key;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_ready) {
        return key;
    }
    return resolveTextLocked(key);
}

std::string LFI18nManager::tr(const std::string& key) {
    return get(key);
}

bool LFI18nManager::ensureInitialized() {
    return ensureInitializedFromAsset("");
}

bool LFI18nManager::ensureInitializedFromAsset(const std::string& assetPath) {
    const std::string resolvedAssetPath = assetPath.empty() ? kDefaultAssetPath : assetPath;

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_loading) {
            m_stateCv.wait(lock, [this]() {
                return !m_loading;
            });
        }

        if (m_ready && m_assetPath == resolvedAssetPath) {
            return true;
        }

        m_loading = true;
        m_assetPath = resolvedAssetPath;
    }

    ParsedTranslations parsed = parseTranslations(loadAssetSynchronously(resolvedAssetPath));
    LFI18nPlatformLocaleResult systemLocale;
    std::string cachedLanguageTag;
    if (parsed.ok) {
        systemLocale = loadSystemLanguageSynchronously();
        cachedLanguageTag = loadCachedLanguage();
    }

    bool ready = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!parsed.ok) {
            clearStateLocked();
            ready = false;
        } else {
            m_translations = std::move(parsed.translations);
            m_languageOrder = std::move(parsed.languageOrder);
            m_defaultLanguageTag = resolveDefaultLanguageTagLocked(
                parsed.defaultLanguageTag.empty() ? kDefaultLanguageTag : parsed.defaultLanguageTag
            );
            m_systemLanguageTag = systemLocale.ok ? systemLocale.locale.toTag() : "";

            const std::string requestedTag = !m_pendingLanguageTag.empty()
                ? m_pendingLanguageTag
                : (!cachedLanguageTag.empty() ? cachedLanguageTag : m_systemLanguageTag);
            if (!applyCurrentLanguageLocked(requestedTag)) {
                m_currentLanguageTag = m_defaultLanguageTag;
            }

            m_ready = true;
            ready = true;
        }

        m_loading = false;
    }
    m_stateCv.notify_all();
    return ready;
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

    const std::filesystem::path cacheFilePath = resolveCacheFilePath();
    if (cacheFilePath.empty()) {
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(cacheFilePath.parent_path(), ec);

    std::ofstream output(cacheFilePath, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << "{\n  \"language\": \"" << normalizedTag << "\"\n}";
    return output.good();
}

std::string LFI18nManager::loadCachedLanguage() const {
    const std::filesystem::path cacheFilePath = resolveCacheFilePath();
    if (cacheFilePath.empty()) {
        return "";
    }

    std::ifstream input(cacheFilePath, std::ios::binary);
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
        auto json = LFJSONParser::parse(content);
        if (!json || !json->contains("language") || !json->at("language").isString()) {
            return "";
        }
        return LFLocale::fromTag(json->at("language").asString()).toTag();
    } catch (...) {
        return "";
    }
}
