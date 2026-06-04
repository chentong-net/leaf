#include "LFI18n.h"

void LFI18n::initialize(LFI18nInitCallback callback) {
    LFI18nManager::getInstance().initialize(std::move(callback));
}

void LFI18n::initializeFromAsset(const std::string& assetPath, LFI18nInitCallback callback) {
    LFI18nManager::getInstance().initializeFromAsset(assetPath, std::move(callback));
}

bool LFI18n::isReady() {
    return LFI18nManager::getInstance().isReady();
}

bool LFI18n::setLanguage(const LFLocale& locale) {
    return LFI18nManager::getInstance().setLanguage(locale);
}

bool LFI18n::setLanguage(const std::string& languageTag) {
    return LFI18nManager::getInstance().setLanguage(languageTag);
}

LFLocale LFI18n::getCurrentLanguage() {
    return LFI18nManager::getInstance().getCurrentLanguage();
}

LFLocale LFI18n::getDefaultLanguage() {
    return LFI18nManager::getInstance().getDefaultLanguage();
}

LFLocale LFI18n::getSystemLanguage() {
    return LFI18nManager::getInstance().getSystemLanguage();
}

std::string LFI18n::tr(const std::string& key) {
    return LFI18nManager::getInstance().tr(key);
}

void LFI18n::bindText(const std::shared_ptr<LFText>& textNode, const std::string& key) {
    LFI18nManager::getInstance().bindText(textNode, key);
}
