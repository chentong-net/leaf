//
// EnglishWords data access and asset path mapping.
//

#include "EnglishWordsDataManager.h"

#include "LFJSONParser.h"
#include "LFPathProvider.h"
#include "LFResourceProvider.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr const char* kLevelTopicsAssetPath = "EnglishWordsAssets/level_topics.json";
constexpr const char* kAudioAssetPrefix = "EnglishWordsAssets/Resources/Sounds/";
constexpr char kBackslashChar = '\\';

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
                callback(false, "", assetPath.empty() ? "asset_load_failed" : "asset_load_failed");
                return;
            }

            std::string text(reinterpret_cast<const char*>(data->data), data->size);
            callback(true, stripUtf8Bom(std::move(text)), "");
        }
    );
}

std::string buildAudioAssetPath(const std::string& folder, int audioId) {
    if (folder.empty() || audioId < 0) {
        return "";
    }

    const std::string audioIdText = audioId < 10
        ? "0" + std::to_string(audioId)
        : std::to_string(audioId);
    return std::string(kAudioAssetPrefix) + folder + "/" + audioIdText + ".mp3";
}

bool parseAudioToken(const std::string& token, std::string& folder, int& audioId) {
    const size_t slashIndex = token.find(kBackslashChar);
    if (slashIndex == std::string::npos) {
        return false;
    }

    folder = trimCopy(token.substr(0, slashIndex));
    const std::string audioIdText = trimCopy(token.substr(slashIndex + 1));
    if (folder.empty() || audioIdText.empty()) {
        return false;
    }

    try {
        audioId = std::stoi(audioIdText);
    } catch (...) {
        return false;
    }

    return audioId >= 0;
}

ParsedWordEntry parseWordLine(const std::string& rawLine) {
    ParsedWordEntry parsed;
    const std::string line = trimCopy(rawLine);
    if (line.empty()) {
        return parsed;
    }

    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= line.size()) {
        const size_t hashIndex = line.find('#', start);
        if (hashIndex == std::string::npos) {
            parts.push_back(trimCopy(line.substr(start)));
            break;
        }

        parts.push_back(trimCopy(line.substr(start, hashIndex - start)));
        start = hashIndex + 1;
    }

    if (parts.empty()) {
        return parsed;
    }

    parsed.entry.text = parts[0];
    if (parts.size() == 1) {
        return parsed;
    }

    if (parts.size() == 2) {
        parsed.entry.translation = parts[1];
        parsed.entry.russianTranslation = parts[1];
        return parsed;
    }

    const std::string& audioToken = parts[1];
    parsed.hasAudio = parseAudioToken(audioToken, parsed.audioFolder, parsed.audioId);
    if (parsed.hasAudio) {
        parsed.entry.audioAssetPath = buildAudioAssetPath(parsed.audioFolder, parsed.audioId);
    }

    if (parsed.hasAudio) {
        parsed.entry.russianTranslation = parts.size() >= 3 ? parts[2] : "";
        parsed.entry.chineseTranslation = parts.size() >= 4 ? parts[3] : "";
    } else {
        parsed.entry.russianTranslation = parts.size() >= 2 ? parts[1] : "";
        parsed.entry.chineseTranslation = parts.size() >= 3 ? parts[2] : "";
    }

    parsed.entry.translation = !parsed.entry.russianTranslation.empty()
        ? parsed.entry.russianTranslation
        : parsed.entry.chineseTranslation;

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
        if (next.audioId != previous.audioId + 2) {
            continue;
        }

        current.audioFolder = previous.audioFolder;
        current.audioId = previous.audioId + 1;
        current.hasAudio = true;
        current.entry.audioAssetPath = buildAudioAssetPath(current.audioFolder, current.audioId);
    }
}

std::vector<EnglishWordEntry> parseEntries(const std::string& text) {
    std::vector<ParsedWordEntry> parsedEntries;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        parsedEntries.push_back(parseWordLine(line));
    }

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

std::string fallbackTemporaryDirectory() {
#if defined(__DESKTOP__)
    std::error_code ec;
    const auto path = std::filesystem::temp_directory_path(ec);
    if (!ec) {
        return path.u8string();
    }
#endif
    return "";
}

bool writeBinaryFile(const std::string& path, const std::shared_ptr<LFData>& data) {
    if (path.empty() || !data || !data->data || data->size == 0) {
        return false;
    }

    const auto filePath = std::filesystem::u8path(path);
    const auto parent = filePath.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return false;
        }
    }

    std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output.write(reinterpret_cast<const char*>(data->data), static_cast<std::streamsize>(data->size));
    return output.good();
}

std::string buildCachedAudioPath(const std::string& directory, const std::string& assetPath) {
    const size_t key = std::hash<std::string>{}(assetPath);
    const auto path = std::filesystem::u8path(directory) /
                      ("leaf_english_words_" + std::to_string(static_cast<unsigned long long>(key)) + ".mp3");
    return path.u8string();
}

} // namespace

EnglishWordsDataManager::Ptr EnglishWordsDataManager::create() {
    return std::make_shared<EnglishWordsDataManager>();
}

void EnglishWordsDataManager::loadLevels(LoadLevelsCallback callback) {
    if (!callback) {
        return;
    }

    fetchTextAsset(
        kLevelTopicsAssetPath,
        [callback = std::move(callback)](bool ok, std::string text, const std::string& error) mutable {
            if (!ok) {
                callback(false, {}, error.empty() ? "level_topics_load_failed" : error);
                return;
            }

            try {
                auto root = LFJSONParser::parse(text);
                if (!root || !root->contains("levels") || !root->at("levels").isArray()) {
                    callback(false, {}, "level_topics_parse_failed");
                    return;
                }

                std::vector<EnglishWordLevel> levels;
                for (auto& levelValue : root->at("levels").asArray()) {
                    if (!levelValue.isObject()) {
                        continue;
                    }

                    auto levelObject = levelValue.asObject();
                    EnglishWordLevel level;
                    level.id = stringField(levelObject, "id");
                    level.title = stringField(levelObject, "title");
                    if (level.title.empty()) {
                        continue;
                    }

                    if (levelObject->contains("topics") && levelObject->at("topics").isArray()) {
                        for (auto& topicValue : levelObject->at("topics").asArray()) {
                            if (!topicValue.isObject()) {
                                continue;
                            }

                            auto topicObject = topicValue.asObject();
                            EnglishWordTopic topic;
                            topic.id = stringField(topicObject, "id");
                            topic.title = stringField(topicObject, "title");
                            topic.fileAssetPath = stringField(topicObject, "file");
                            if (!topic.title.empty() && !topic.fileAssetPath.empty()) {
                                level.topics.push_back(std::move(topic));
                            }
                        }
                    }

                    levels.push_back(std::move(level));
                }

                callback(true, std::move(levels), "");
            } catch (...) {
                callback(false, {}, "level_topics_parse_failed");
            }
        }
    );
}

void EnglishWordsDataManager::loadEntries(const EnglishWordTopic& topic, LoadEntriesCallback callback) {
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
                callback(false, {}, error.empty() ? "topic_load_failed" : error);
                return;
            }
            callback(true, parseEntries(text), "");
        }
    );
}

void EnglishWordsDataManager::resolveAudioPath(const std::string& audioAssetPath, ResolveAudioPathCallback callback) {
    if (!callback) {
        return;
    }
    if (audioAssetPath.empty()) {
        callback(false, "", "audio_path_missing");
        return;
    }

    auto cached = m_cachedAudioPaths.find(audioAssetPath);
    if (cached != m_cachedAudioPaths.end()) {
        std::error_code ec;
        if (std::filesystem::exists(std::filesystem::u8path(cached->second), ec) && !ec) {
            callback(true, cached->second, "");
            return;
        }
    }

    std::weak_ptr<EnglishWordsDataManager> weakSelf = weak_from_this();
    resolveTempDirectory(
        [weakSelf, audioAssetPath, callback = std::move(callback)](const std::string& directory) mutable {
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }
            if (directory.empty()) {
                callback(false, "", "temp_directory_unavailable");
                return;
            }

            LFResourceProvider::getInstance().fetchAsset(
                audioAssetPath,
                [weakSelf, audioAssetPath, directory, callback = std::move(callback)](std::shared_ptr<LFData> data) mutable {
                    auto self = weakSelf.lock();
                    if (!self) {
                        return;
                    }
                    if (!data || !data->data || data->size == 0) {
                        callback(false, "", "audio_asset_load_failed");
                        return;
                    }

                    const std::string path = buildCachedAudioPath(directory, audioAssetPath);
                    if (!writeBinaryFile(path, data)) {
                        callback(false, "", "audio_file_write_failed");
                        return;
                    }

                    self->m_cachedAudioPaths[audioAssetPath] = path;
                    callback(true, path, "");
                }
            );
        }
    );
}

void EnglishWordsDataManager::resolveTempDirectory(DirectoryCallback callback) {
    if (!callback) {
        return;
    }

    if (!m_tempDirectory.empty()) {
        callback(m_tempDirectory);
        return;
    }

    m_pendingDirectoryCallbacks.push_back(std::move(callback));
    if (m_resolvingTempDirectory) {
        return;
    }

    const std::string fallback = fallbackTemporaryDirectory();
    if (!fallback.empty()) {
        m_tempDirectory = fallback;
        auto callbacks = std::move(m_pendingDirectoryCallbacks);
        m_pendingDirectoryCallbacks.clear();
        for (auto& current : callbacks) {
            if (current) {
                current(m_tempDirectory);
            }
        }
        return;
    }

    m_resolvingTempDirectory = true;
    std::weak_ptr<EnglishWordsDataManager> weakSelf = weak_from_this();
    LFPathProvider::getTemporaryPath([weakSelf](const LFPathProviderResult& result) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }

        self->m_resolvingTempDirectory = false;
        self->m_tempDirectory = result.ok ? result.path : "";

        auto callbacks = std::move(self->m_pendingDirectoryCallbacks);
        self->m_pendingDirectoryCallbacks.clear();
        for (auto& current : callbacks) {
            if (current) {
                current(self->m_tempDirectory);
            }
        }
    });
}
