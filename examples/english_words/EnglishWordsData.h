//
// Shared EnglishWords data models and loaders.
//

#ifndef ENGLISHWORDS_DATA_H
#define ENGLISHWORDS_DATA_H

#include <functional>
#include <string>
#include <vector>

struct EnglishWordTopic {
    std::string levelId;
    std::string levelTitle;
    std::string topicId;
    std::string title;
    std::string fileAssetPath;
};

struct EnglishWordLevel {
    std::string id;
    std::string title;
    std::vector<EnglishWordTopic> topics;
};

struct EnglishWordEntry {
    std::string text;
    std::string translation;
    std::string audioAssetPath;
};

using EnglishWordLevelsCallback =
    std::function<void(bool ok, std::vector<EnglishWordLevel> levels, const std::string& error)>;

using EnglishWordEntriesCallback =
    std::function<void(bool ok, std::vector<EnglishWordEntry> entries, const std::string& error)>;

void loadEnglishWordLevels(EnglishWordLevelsCallback callback);
void loadEnglishWordEntries(const EnglishWordTopic& topic, EnglishWordEntriesCallback callback);

#endif // ENGLISHWORDS_DATA_H
