#include "LFI18nPlatform.h"
#include "LFJSONParser.h"
#include "plugin/LFNativeSender.h"

void LFI18nPlatform::getSystemLanguage(LFI18nPlatformLocaleCallback callback) {
    if (!callback) {
        return;
    }

    LFNativeSender::getInstance().send("i18n.get_system_language", "{}", [callback = std::move(callback)](const LFMethodResult& result) mutable {
        LFI18nPlatformLocaleResult out;
        out.ok = false;

        if (!result.ok) {
            out.error = result.error.empty() ? "get_system_language_failed" : result.error;
            callback(out);
            return;
        }

        try {
            auto json = LFJSONParser::parse(result.data);
            std::string languageTag;
            if (json->contains("languageTag")) {
                languageTag = json->at("languageTag").asString();
            }

            out.locale = LFLocale::fromTag(languageTag);
            out.ok = !out.locale.isEmpty();
            if (!out.ok) {
                out.error = "system_language_empty";
            }
        } catch (...) {
            out.error = "system_language_parse_failed";
        }

        callback(out);
    });
}
