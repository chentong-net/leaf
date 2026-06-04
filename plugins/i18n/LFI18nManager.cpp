#include "LFI18nManager.h"
#include "LFI18nPlatform.h"
#include "LFJSONParser.h"
#include "plugin/LFPlugin.h"

namespace {

bool localeMatches(const LFLocale& lhs, const LFLocale& rhs) {
    return lhs.toTag() == rhs.toTag();
}

bool localeLanguageMatches(const LFLocale& lhs, const LFLocale& rhs) {
    return !lhs.languageCode.empty() && lhs.languageCode == rhs.languageCode;
}

} // namespace

LFI18nManager& LFI18nManager::getInstance() {
    static LFI18nManager instance;
    return instance;
}

void LFI18nManager::initialize(LFI18nInitCallback callback) {
    initializeFromAsset("i18n.json", std::move(callback));
}

void LFI18nManager::initializeFromAsset(const std::string& assetPath, LFI18nInitCallback callback) {
    std::string resolvedAssetPath = assetPath.empty() ? "i18n.json" : assetPath;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (callback) {
            m_pendingCallbacks.push_back(std::move(callback));
        }

        if (m_loading && m_assetPath == resolvedAssetPath) {
            return;
        }

        if (m_ready && m_assetPath == resolvedAssetPath) {
            auto callbacks = std::move(m_pendingCallbacks);
            m_pendingCallbacks.clear();
            for (const auto& pending : callbacks) {
                if (pending) {
                    LFPluginCenter::dispatchToMain([pending]() {
                        pending(true);
                    });
                }
            }
            return;
        }

        m_assetPath = resolvedAssetPath;
        m_loading = true;
        m_ready = false;
    }

    LFResourceProvider::getInstance().fetchAsset(resolvedAssetPath, [this](std::shared_ptr<LFData> data) {
        ParsedTranslations parsed = parseTranslations(data);
        LFI18nPlatform::getSystemLanguage([this, parsed = std::move(parsed)](const LFI18nPlatformLocaleResult& platformLocale) mutable {
            std::vector<LFI18nInitCallback> callbacks;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                callbacks = std::move(m_pendingCallbacks);
                m_pendingCallbacks.clear();
            }

            LFPluginCenter::dispatchToMain([this,
                                            parsed = std::move(parsed),
                                            platformLocale,
                                            callbacks = std::move(callbacks)]() mutable {
                finalizeInitialize(std::move(parsed), platformLocale, std::move(callbacks));
            });
        });
    });
}

bool LFI18nManager::isReady() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ready;
}

bool LFI18nManager::setLanguage(const LFLocale& locale) {
    return setLanguage(locale.toTag());
}

bool LFI18nManager::setLanguage(const std::string& languageTag) {
    bool changed = false;
    bool success = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingLanguageTag = LFLocale::fromTag(languageTag).toTag();
        success = applyCurrentLanguage(m_pendingLanguageTag);
        changed = success;
    }

    if (changed) {
        refreshBindings();
    }
    return success;
}

LFLocale LFI18nManager::getCurrentLanguage() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return LFLocale::fromTag(m_currentLanguageTag);
}

LFLocale LFI18nManager::getDefaultLanguage() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return LFLocale::fromTag(m_defaultLanguageTag);
}

LFLocale LFI18nManager::getSystemLanguage() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return LFLocale::fromTag(m_systemLanguageTag);
}

std::string LFI18nManager::tr(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_ready) {
        return key;
    }
    return resolveTextLocked(key);
}

void LFI18nManager::bindText(const std::shared_ptr<LFText>& textNode, const std::string& key) {
    if (!textNode || key.empty()) {
        return;
    }

    std::string value;
    bool shouldApplyNow = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_bindings.push_back({textNode, key});
        shouldApplyNow = m_ready;
        if (shouldApplyNow) {
            value = resolveTextLocked(key);
        }
    }

    if (shouldApplyNow) {
        textNode->setText(value);
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

void LFI18nManager::finalizeInitialize(
        ParsedTranslations parsed,
        LFI18nPlatformLocaleResult platformLocale,
        std::vector<LFI18nInitCallback> callbacks) {
    bool ready = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_loading = false;

        if (!parsed.ok) {
            m_translations.clear();
            m_languageOrder.clear();
            m_ready = false;
            ready = false;
        } else {
            m_translations = std::move(parsed.translations);
            m_languageOrder = std::move(parsed.languageOrder);
            m_defaultLanguageTag = resolveDefaultLanguageTagLocked(
                parsed.defaultLanguageTag.empty() ? "zh-CN" : parsed.defaultLanguageTag
            );
            m_systemLanguageTag = platformLocale.ok
                ? resolveAvailableLanguageTagLocked(platformLocale.locale.toTag())
                : "";

            const std::string requestedTag = !m_pendingLanguageTag.empty()
                ? m_pendingLanguageTag
                : m_systemLanguageTag;
            if (!applyCurrentLanguage(requestedTag)) {
                m_currentLanguageTag = m_defaultLanguageTag;
            }

            m_ready = true;
            ready = true;
        }
    }

    if (ready) {
        refreshBindings();
    }

    for (const auto& callback : callbacks) {
        if (callback) {
            callback(ready);
        }
    }
}

bool LFI18nManager::applyCurrentLanguage(const std::string& requestedTag) {
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

void LFI18nManager::refreshBindings() {
    std::vector<std::pair<std::shared_ptr<LFText>, std::string>> updates;
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto bindingIt = m_bindings.begin();
        while (bindingIt != m_bindings.end()) {
            auto textNode = bindingIt->textNode.lock();
            if (!textNode) {
                bindingIt = m_bindings.erase(bindingIt);
                continue;
            }

            updates.emplace_back(textNode, resolveTextLocked(bindingIt->key));
            ++bindingIt;
        }
    }

    for (const auto& update : updates) {
        update.first->setText(update.second);
    }
}
