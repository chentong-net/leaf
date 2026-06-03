//
// EnglishWords asset-backed data loading helpers.
//

#include "EnglishWordsData.h"

#include "LFJSONParser.h"
#include "LFResourceProvider.h"

#include <cctype>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr const char* kLevelTopicsAssetPath = "EnglishWordsAssets/level_topics.json";
constexpr const char* kAudioAssetPrefix = "EnglishWordsAssets/Resources/Sounds/";
constexpr const char kBackslashChar = '\\';

struct ParsedWordEntry {
    EnglishWordEntry entry;
    std::string audioFolder;
    int audioId = -1;
    bool hasAudio = false;
};

using TextAssetCallback = std::function<void(bool ok, std::string text, const std::string& error)>;

std::string trimCopy(const std::string& value) {
    size_t start = 0;
    size_t end = value.size();

    while (start < end && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return value.substr(start, end - start);
}

std::string stripUtf8Bom(std::string text) {
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.erase(0, 3);
    }
    return text;
}

std::string stringField(const LFJSONObject::Ptr& object, const char* key) {
    if (!object || !key || !object->contains(key)) {
        return "";
    }
    try {
        return object->at(key).asString();
    } catch (...) {
        return "";
    }
}

void fetchTextAsset(const std::string& assetPath, TextAssetCallback callback) {
    if (!callback) {
        return;
    }

    LFResourceProvider::getInstance().fetchAsset(
        assetPath,
        [assetPath, callback = std::move(callback)](std::shared_ptr<LFData> data) mutable {
            if (!data || !data->data || data->size == 0) {
                callback(false, "", assetPath.empty() ? "asset_load_failed" : assetPath + ":asset_load_failed");
                return;
            }

            std::string text(reinterpret_cast<const char*>(data->data), data->size);
            text = stripUtf8Bom(std::move(text));
            callback(true, std::move(text), "");
        }
    );
}

std::string buildAudioAssetPath(const std::string& folder, int audioId) {
    if (folder.empty() || audioId < 0) {
        return "";
    }
    std::string audioIdStr = "";
    if (audioId < 10) {
        audioIdStr = "0" + std::to_string(audioId);
    } else {
        audioIdStr = std::to_string(audioId);
    }
    return std::string(kAudioAssetPrefix) + folder + "/" + audioIdStr + ".mp3";
}

bool parseAudioToken(const std::string& token, std::string& outFolder, int& outAudioId) {
    const size_t slashIndex = token.find(kBackslashChar);
    if (slashIndex == std::string::npos) {
        return false;
    }

    const std::string folder = trimCopy(token.substr(0, slashIndex));
    const std::string audioIdText = trimCopy(token.substr(slashIndex + 1));
    if (folder.empty() || audioIdText.empty()) {
        return false;
    }

    try {
        outAudioId = std::stoi(audioIdText);
    } catch (...) {
        return false;
    }

    outFolder = folder;
    return outAudioId >= 0;
}

ParsedWordEntry parseWordLine(const std::string& rawLine) {
    ParsedWordEntry parsed;
    const std::string line = trimCopy(rawLine);
    if (line.empty()) {
        return parsed;
    }

    const size_t firstHash = line.find('#');
    if (firstHash == std::string::npos) {
        parsed.entry.text = line;
        return parsed;
    }

    const size_t secondHash = line.find('#', firstHash + 1);
    parsed.entry.text = trimCopy(line.substr(0, firstHash));

    if (secondHash == std::string::npos) {
        parsed.entry.translation = trimCopy(line.substr(firstHash + 1));
        return parsed;
    }

    const std::string audioToken = trimCopy(line.substr(firstHash + 1, secondHash - firstHash - 1));
    parsed.entry.translation = trimCopy(line.substr(secondHash + 1));

    parsed.hasAudio = parseAudioToken(audioToken, parsed.audioFolder, parsed.audioId);
    if (parsed.hasAudio) {
        parsed.entry.audioAssetPath = buildAudioAssetPath(parsed.audioFolder, parsed.audioId);
    }

    return parsed;
}

void repairMissingAudio(std::vector<ParsedWordEntry>& entries) {
    if (entries.size() < 3) {
        return;
    }

    for (size_t index = 1; index + 1 < entries.size(); ++index) {
        auto& current = entries[index];
        if (current.hasAudio || current.entry.text.empty() || current.entry.translation.empty()) {
            continue;
        }

        const auto& previous = entries[index - 1];
        const auto& next = entries[index + 1];
        if (!previous.hasAudio || !next.hasAudio) {
            continue;
        }
        if (previous.audioFolder.empty() || previous.audioFolder != next.audioFolder) {
            continue;
        }
        if (previous.audioId < 0 || next.audioId != previous.audioId + 2) {
            continue;
        }

        current.audioFolder = previous.audioFolder;
        current.audioId = previous.audioId + 1;
        current.hasAudio = true;
        current.entry.audioAssetPath = buildAudioAssetPath(current.audioFolder, current.audioId);
    }
}

std::vector<EnglishWordEntry> flattenEntries(std::vector<ParsedWordEntry> parsedEntries) {
    repairMissingAudio(parsedEntries);

    std::vector<EnglishWordEntry> entries;
    entries.reserve(parsedEntries.size());
    for (auto& parsed : parsedEntries) {
        if (parsed.entry.text.empty() && parsed.entry.translation.empty()) {
            continue;
        }
        entries.push_back(std::move(parsed.entry));
    }
    return entries;
}

} // namespace

void loadEnglishWordLevels(EnglishWordLevelsCallback callback) {
    if (!callback) {
        return;
    }

    fetchTextAsset(
        kLevelTopicsAssetPath,
        [callback = std::move(callback)](bool ok, std::string text, const std::string& error) mutable {
            if (!ok) {
                callback(false, {}, error);
                return;
            }

            try {
                auto root = LFJSONParser::parse(text);
                if (!root || !root->contains("levels")) {
                    callback(false, {}, "level_topics_missing_levels");
                    return;
                }

                auto& levelsValue = root->at("levels");
                if (!levelsValue.isArray()) {
                    callback(false, {}, "level_topics_invalid_levels");
                    return;
                }

                std::vector<EnglishWordLevel> levels;
                for (auto& levelValue : levelsValue.asArray()) {
                    if (!levelValue.isObject()) {
                        continue;
                    }

                    auto levelObject = levelValue.asObject();
                    EnglishWordLevel level;
                    level.id = stringField(levelObject, "id");
                    level.title = stringField(levelObject, "title");

                    if (levelObject->contains("topics")) {
                        auto& topicsValue = levelObject->at("topics");
                        if (topicsValue.isArray()) {
                            for (auto& topicValue : topicsValue.asArray()) {
                                if (!topicValue.isObject()) {
                                    continue;
                                }

                                auto topicObject = topicValue.asObject();
                                EnglishWordTopic topic;
                                topic.levelId = level.id;
                                topic.levelTitle = level.title;
                                topic.topicId = stringField(topicObject, "id");
                                topic.title = stringField(topicObject, "title");
                                topic.fileAssetPath = stringField(topicObject, "file");

                                if (!topic.title.empty() && !topic.fileAssetPath.empty()) {
                                    level.topics.push_back(std::move(topic));
                                }
                            }
                        }
                    }

                    if (!level.title.empty()) {
                        levels.push_back(std::move(level));
                    }
                }

                callback(true, std::move(levels), "");
            } catch (...) {
                callback(false, {}, "level_topics_parse_failed");
            }
        }
    );
}

void loadEnglishWordEntries(const EnglishWordTopic& topic, EnglishWordEntriesCallback callback) {
    if (!callback) {
        return;
    }

    if (topic.fileAssetPath.empty()) {
        callback(false, {}, "topic_file_missing");
        return;
    }

    fetchTextAsset(
        topic.fileAssetPath,
        [callback = std::move(callback)](bool ok, std::string text, const std::string& error) mutable {
            if (!ok) {
                callback(false, {}, error);
                return;
            }

            std::vector<ParsedWordEntry> parsedEntries;
            std::istringstream input(text);
            std::string line;
            while (std::getline(input, line)) {
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                parsedEntries.push_back(parseWordLine(line));
            }

            callback(true, flattenEntries(std::move(parsedEntries)), "");
        }
    );
}
