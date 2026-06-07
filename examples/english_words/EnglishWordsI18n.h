//
// EnglishWords localized string helpers.
//

#ifndef ENGLISHWORDS_I18N_H
#define ENGLISHWORDS_I18N_H

#include "EnglishWordsDataManager.h"
#include "LFI18n.h"

#include <cstddef>
#include <initializer_list>
#include <string>
#include <utility>

namespace EnglishWordsI18n {

inline std::string tr(const std::string& key) {
    return LFI18n::get(key);
}

inline void replaceAll(std::string& value, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return;
    }

    size_t start = 0;
    while ((start = value.find(from, start)) != std::string::npos) {
        value.replace(start, from.size(), to);
        start += to.size();
    }
}

inline std::string formatPattern(
    std::string pattern,
    const std::initializer_list<std::pair<std::string, std::string>>& replacements
) {
    for (const auto& replacement : replacements) {
        replaceAll(pattern, "{" + replacement.first + "}", replacement.second);
    }
    return pattern;
}

inline std::string format(
    const std::string& key,
    const std::initializer_list<std::pair<std::string, std::string>>& replacements
) {
    return formatPattern(tr(key), replacements);
}

inline std::string modeLabel(EnglishWordsTestMode mode) {
    switch (mode) {
        case EnglishWordsTestMode::ChineseToEnglish:
            return tr("english_words.mode.chinese_to_english");
        case EnglishWordsTestMode::RussianToEnglish:
            return tr("english_words.mode.russian_to_english");
        case EnglishWordsTestMode::EnglishToChinese:
            return tr("english_words.mode.english_to_chinese");
        case EnglishWordsTestMode::EnglishToRussian:
            return tr("english_words.mode.english_to_russian");
        case EnglishWordsTestMode::AudioToChinese:
            return tr("english_words.mode.audio_to_chinese");
        case EnglishWordsTestMode::AudioToRussian:
            return tr("english_words.mode.audio_to_russian");
        case EnglishWordsTestMode::AudioToEnglish:
            return tr("english_words.mode.audio_to_english");
    }

    return tr("english_words.mode.exam");
}

inline std::string topicsCount(size_t count) {
    return format("english_words.common.topics_count", {
        {"count", std::to_string(count)}
    });
}

inline std::string levelTitle(const std::string& levelId) {
    return levelId.empty()
        ? ""
        : format("english_words.common.level_title", {
            {"id", levelId}
        });
}

inline std::string audioPrompt(const std::string& value) {
    return format("english_words.common.audio_prompt", {
        {"value", value}
    });
}

inline std::string questionProgress(int current, int total) {
    return format("english_words.exam.question_progress", {
        {"current", std::to_string(current)},
        {"total", std::to_string(total)}
    });
}

inline std::string scoreRow(const std::string& score, int total) {
    return format("english_words.result.score_row", {
        {"score", score},
        {"total", std::to_string(total)}
    });
}

inline std::string questionResultLine(int index, const std::string& prompt, const std::string& status) {
    return format("english_words.point.question_result", {
        {"index", std::to_string(index)},
        {"prompt", prompt},
        {"status", status}
    });
}

inline std::string yourAnswer(const std::string& answer) {
    return format("english_words.point.your_answer", {
        {"answer", answer}
    });
}

} // namespace EnglishWordsI18n

#endif // ENGLISHWORDS_I18N_H
