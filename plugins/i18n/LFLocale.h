#ifndef LEAF_LFLOCALE_H
#define LEAF_LFLOCALE_H

#include "LFDef.h"

namespace lf_i18n_detail {

inline std::string toLowerAscii(std::string value) {
    for (char& ch : value) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return value;
}

inline std::string toUpperAscii(std::string value) {
    for (char& ch : value) {
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    return value;
}

inline std::string toTitleAscii(std::string value) {
    if (value.empty()) {
        return value;
    }
    value = toLowerAscii(std::move(value));
    char& first = value[0];
    if (first >= 'a' && first <= 'z') {
        first = static_cast<char>(first - 'a' + 'A');
    }
    return value;
}

inline std::vector<std::string> splitLocaleTag(const std::string& rawTag) {
    std::vector<std::string> parts;
    std::string current;
    current.reserve(rawTag.size());

    for (char ch : rawTag) {
        if (ch == '-' || ch == '_') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(ch);
    }

    if (!current.empty()) {
        parts.push_back(current);
    }
    return parts;
}

} // namespace lf_i18n_detail

struct LFLocale {
    std::string languageCode;
    std::string countryCode;
    std::string scriptCode;

    bool isEmpty() const {
        return languageCode.empty() && countryCode.empty() && scriptCode.empty();
    }

    std::string toTag() const {
        if (languageCode.empty()) {
            return "";
        }

        std::string tag = lf_i18n_detail::toLowerAscii(languageCode);
        if (!scriptCode.empty()) {
            tag += "-";
            tag += lf_i18n_detail::toTitleAscii(scriptCode);
        }
        if (!countryCode.empty()) {
            tag += "-";
            tag += lf_i18n_detail::toUpperAscii(countryCode);
        }
        return tag;
    }

    static LFLocale fromTag(const std::string& rawTag) {
        LFLocale locale;
        const std::vector<std::string> parts = lf_i18n_detail::splitLocaleTag(rawTag);
        if (parts.empty()) {
            return locale;
        }

        locale.languageCode = lf_i18n_detail::toLowerAscii(parts[0]);
        size_t nextIndex = 1;
        if (parts.size() > nextIndex && parts[nextIndex].size() == 4) {
            locale.scriptCode = lf_i18n_detail::toTitleAscii(parts[nextIndex]);
            nextIndex++;
        }
        if (parts.size() > nextIndex &&
            (parts[nextIndex].size() == 2 || parts[nextIndex].size() == 3)) {
            locale.countryCode = lf_i18n_detail::toUpperAscii(parts[nextIndex]);
        }
        return locale;
    }

    bool operator==(const LFLocale& other) const {
        return toTag() == other.toTag();
    }

    bool operator!=(const LFLocale& other) const {
        return !(*this == other);
    }
};

namespace LFLocales {

inline const LFLocale ZhCN{"zh", "CN", ""};
inline const LFLocale EnUS{"en", "US", ""};
inline const LFLocale RuRU{"ru", "RU", ""};

inline const LFLocale Zh{"zh", "", ""};
inline const LFLocale En{"en", "", ""};
inline const LFLocale Ru{"ru", "", ""};

} // namespace LFLocales

#endif // LEAF_LFLOCALE_H
