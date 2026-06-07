//
// EnglishWords data access and asset path mapping.
//

#include "EnglishWordsDataManager.h"

#include "LFJSONParser.h"
#include "LFPathProvider.h"
#include "LFResourceProvider.h"

#include <chrono>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
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
using LoadLevelsCallback = EnglishWordsDataManager::LoadLevelsCallback;

enum class SharedLevelsCacheState {
    Empty,
    Loading,
    Ready,
};

struct SharedLevelsCache {
    SharedLevelsCacheState state = SharedLevelsCacheState::Empty;
    std::vector<EnglishWordLevel> levels;
    std::vector<LoadLevelsCallback> pendingCallbacks;
};

SharedLevelsCache& sharedLevelsCache() {
    static SharedLevelsCache cache;
    return cache;
}

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

bool parseLevelsJson(const std::string& text,
                     std::vector<EnglishWordLevel>& levels,
                     std::string& error) {
    try {
        auto root = LFJSONParser::parse(text);
        if (!root || !root->contains("levels") || !root->at("levels").isArray()) {
            error = "level_topics_parse_failed";
            return false;
        }

        std::vector<EnglishWordLevel> parsedLevels;
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

            parsedLevels.push_back(std::move(level));
        }

        levels = std::move(parsedLevels);
        error.clear();
        return true;
    } catch (...) {
        error = "level_topics_parse_failed";
        return false;
    }
}

void finishSharedLevelsLoad(bool ok, std::vector<EnglishWordLevel> levels, const std::string& error) {
    auto& cache = sharedLevelsCache();
    cache.state = ok ? SharedLevelsCacheState::Ready : SharedLevelsCacheState::Empty;
    cache.levels = ok ? std::move(levels) : std::vector<EnglishWordLevel>{};

    auto callbacks = std::move(cache.pendingCallbacks);
    cache.pendingCallbacks.clear();
    for (auto& callback : callbacks) {
        if (!callback) {
            continue;
        }
        callback(ok, ok ? cache.levels : std::vector<EnglishWordLevel>{}, error);
    }
}

void beginSharedLevelsLoad() {
    auto& cache = sharedLevelsCache();
    if (cache.state == SharedLevelsCacheState::Loading) {
        return;
    }

    cache.state = SharedLevelsCacheState::Loading;
    fetchTextAsset(
        kLevelTopicsAssetPath,
        [](bool ok, std::string text, const std::string& error) {
            if (!ok) {
                finishSharedLevelsLoad(false, {}, error.empty() ? "level_topics_load_failed" : error);
                return;
            }

            std::vector<EnglishWordLevel> levels;
            std::string parseError;
            if (!parseLevelsJson(text, levels, parseError)) {
                finishSharedLevelsLoad(false, {}, parseError.empty() ? "level_topics_parse_failed" : parseError);
                return;
            }

            finishSharedLevelsLoad(true, std::move(levels), "");
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

std::string numberToJson(double value) {
    std::ostringstream stream;
    stream << std::setprecision(15) << value;
    return stream.str();
}

std::string escapeJsonString(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);

    for (char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped.push_back(ch); break;
        }
    }

    return escaped;
}

std::string jsonString(const std::string& value) {
    return "\"" + escapeJsonString(value) + "\"";
}

std::string stringOrEmpty(const LFJSONObject::Ptr& object, const char* key) {
    return stringField(object, key);
}

double numberField(const LFJSONObject::Ptr& object, const char* key, double fallback = 0.0) {
    if (!object || !key || !object->contains(key)) {
        return fallback;
    }

    try {
        return object->at(key).asDouble();
    } catch (...) {
        return fallback;
    }
}

int intField(const LFJSONObject::Ptr& object, const char* key, int fallback = 0) {
    return static_cast<int>(numberField(object, key, static_cast<double>(fallback)));
}

std::string extractLevelIdFromTopicId(const std::string& topicId) {
    if (topicId.empty()) {
        return "";
    }

    const size_t dotIndex = topicId.find('.');
    if (dotIndex == std::string::npos) {
        return topicId;
    }

    return topicId.substr(0, dotIndex);
}

std::string fallbackLevelTitle(const std::string& levelId) {
    return levelId.empty() ? "" : ("Level " + levelId);
}

void populateLevelMetadata(EnglishWordsExamResult& result) {
    if (result.levelId.empty()) {
        result.levelId = extractLevelIdFromTopicId(result.topic.id);
    }

    if (!result.levelTitle.empty()) {
        return;
    }

    const auto& cache = sharedLevelsCache();
    for (const auto& level : cache.levels) {
        if (!result.levelId.empty() && level.id == result.levelId) {
            result.levelTitle = level.title;
            return;
        }

        for (const auto& topic : level.topics) {
            if (topic.id == result.topic.id) {
                result.levelId = level.id;
                result.levelTitle = level.title;
                return;
            }
        }
    }

    result.levelTitle = fallbackLevelTitle(result.levelId);
}

std::string formatTimestamp(const std::chrono::system_clock::time_point& timePoint) {
    const std::time_t timeValue = std::chrono::system_clock::to_time_t(timePoint);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &timeValue);
#else
    localtime_r(&timeValue, &localTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

std::string buildResultId(const std::chrono::system_clock::time_point& timePoint) {
    const std::time_t timeValue = std::chrono::system_clock::to_time_t(timePoint);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &timeValue);
#else
    localtime_r(&timeValue, &localTime);
#endif

    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count() % 1000;

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y%m%d_%H%M%S")
           << "_"
           << std::setw(3) << std::setfill('0') << milliseconds;
    return stream.str();
}

void prepareExamResultForSave(EnglishWordsExamResult& result) {
    if (result.createdAt.empty() || result.resultId.empty()) {
        const auto now = std::chrono::system_clock::now();
        if (result.createdAt.empty()) {
            result.createdAt = formatTimestamp(now);
        }
        if (result.resultId.empty()) {
            result.resultId = buildResultId(now);
        }
    }

    populateLevelMetadata(result);

    if (result.questionCount <= 0) {
        result.questionCount = static_cast<int>(result.questions.size());
    }
    if (result.fileName.empty()) {
        result.fileName = "result_" + result.resultId + ".json";
    }
}

EnglishWordsSavedResultSummary buildSavedResultSummary(const EnglishWordsExamResult& result) {
    EnglishWordsSavedResultSummary summary;
    summary.resultId = result.resultId;
    summary.fileName = result.fileName;
    summary.createdAt = result.createdAt;
    summary.levelId = result.levelId;
    summary.levelTitle = result.levelTitle;
    summary.topic = result.topic;
    summary.mode = result.mode;
    summary.totalScore = result.totalScore;
    summary.questionCount = result.questionCount;
    return summary;
}

std::string serializeWordEntry(const EnglishWordEntry& entry) {
    return "{"
           "\"text\":" + jsonString(entry.text) + ","
           "\"translation\":" + jsonString(entry.translation) + ","
           "\"russianTranslation\":" + jsonString(entry.russianTranslation) + ","
           "\"chineseTranslation\":" + jsonString(entry.chineseTranslation) + ","
           "\"audioAssetPath\":" + jsonString(entry.audioAssetPath) +
           "}";
}

std::string serializeQuestionResult(const EnglishWordsQuestionResult& question) {
    return "{"
           "\"entry\":" + serializeWordEntry(question.entry) + ","
           "\"mode\":" + std::to_string(static_cast<int>(question.mode)) + ","
           "\"userAnswer\":" + jsonString(question.userAnswer) + ","
           "\"score\":" + numberToJson(question.score) +
           "}";
}

std::string serializeExamResultJson(const EnglishWordsExamResult& result) {
    std::ostringstream stream;
    stream << "{\n"
           << "  \"version\": 1,\n"
           << "  \"resultId\": " << jsonString(result.resultId) << ",\n"
           << "  \"fileName\": " << jsonString(result.fileName) << ",\n"
           << "  \"createdAt\": " << jsonString(result.createdAt) << ",\n"
           << "  \"levelId\": " << jsonString(result.levelId) << ",\n"
           << "  \"levelTitle\": " << jsonString(result.levelTitle) << ",\n"
           << "  \"topic\": {\n"
           << "    \"id\": " << jsonString(result.topic.id) << ",\n"
           << "    \"title\": " << jsonString(result.topic.title) << ",\n"
           << "    \"file\": " << jsonString(result.topic.fileAssetPath) << "\n"
           << "  },\n"
           << "  \"mode\": " << std::to_string(static_cast<int>(result.mode)) << ",\n"
           << "  \"totalScore\": " << numberToJson(result.totalScore) << ",\n"
           << "  \"questionCount\": " << result.questionCount << ",\n"
           << "  \"questions\": [";

    for (size_t index = 0; index < result.questions.size(); ++index) {
        if (index > 0) {
            stream << ",";
        }
        stream << "\n    " << serializeQuestionResult(result.questions[index]);
    }

    if (!result.questions.empty()) {
        stream << "\n";
    }

    stream << "  ]\n"
           << "}";
    return stream.str();
}

std::string serializeSavedResultsIndexJson(const std::vector<EnglishWordsSavedResultSummary>& results) {
    std::ostringstream stream;
    stream << "{\n"
           << "  \"version\": 1,\n"
           << "  \"results\": [";

    for (size_t index = 0; index < results.size(); ++index) {
        const auto& result = results[index];
        if (index > 0) {
            stream << ",";
        }
        stream << "\n    {"
               << "\"resultId\": " << jsonString(result.resultId) << ","
               << "\"fileName\": " << jsonString(result.fileName) << ","
               << "\"createdAt\": " << jsonString(result.createdAt) << ","
               << "\"levelId\": " << jsonString(result.levelId) << ","
               << "\"levelTitle\": " << jsonString(result.levelTitle) << ","
               << "\"topicId\": " << jsonString(result.topic.id) << ","
               << "\"topicTitle\": " << jsonString(result.topic.title) << ","
               << "\"topicFile\": " << jsonString(result.topic.fileAssetPath) << ","
               << "\"mode\": " << std::to_string(static_cast<int>(result.mode)) << ","
               << "\"totalScore\": " << numberToJson(result.totalScore) << ","
               << "\"questionCount\": " << result.questionCount
               << "}";
    }

    if (!results.empty()) {
        stream << "\n";
    }

    stream << "  ]\n"
           << "}";
    return stream.str();
}

EnglishWordEntry parseWordEntryObject(const LFJSONObject::Ptr& object) {
    EnglishWordEntry entry;
    entry.text = stringOrEmpty(object, "text");
    entry.translation = stringOrEmpty(object, "translation");
    entry.russianTranslation = stringOrEmpty(object, "russianTranslation");
    entry.chineseTranslation = stringOrEmpty(object, "chineseTranslation");
    entry.audioAssetPath = stringOrEmpty(object, "audioAssetPath");
    if (entry.translation.empty()) {
        entry.translation = !entry.russianTranslation.empty() ? entry.russianTranslation : entry.chineseTranslation;
    }
    return entry;
}

bool parseSavedResultsIndexJson(const std::string& text,
                                std::vector<EnglishWordsSavedResultSummary>& results,
                                std::string& error) {
    try {
        auto root = LFJSONParser::parse(text);
        if (!root || !root->contains("results") || !root->at("results").isArray()) {
            error = "test_results_parse_failed";
            return false;
        }

        std::vector<EnglishWordsSavedResultSummary> parsedResults;
        for (auto& value : root->at("results").asArray()) {
            if (!value.isObject()) {
                continue;
            }

            auto object = value.asObject();
            EnglishWordsSavedResultSummary summary;
            summary.resultId = stringOrEmpty(object, "resultId");
            summary.fileName = stringOrEmpty(object, "fileName");
            summary.createdAt = stringOrEmpty(object, "createdAt");
            summary.levelId = stringOrEmpty(object, "levelId");
            summary.levelTitle = stringOrEmpty(object, "levelTitle");
            summary.topic.id = stringOrEmpty(object, "topicId");
            summary.topic.title = stringOrEmpty(object, "topicTitle");
            summary.topic.fileAssetPath = stringOrEmpty(object, "topicFile");
            summary.mode = static_cast<EnglishWordsTestMode>(intField(object, "mode"));
            summary.totalScore = numberField(object, "totalScore");
            summary.questionCount = intField(object, "questionCount");

            if (!summary.fileName.empty()) {
                parsedResults.push_back(std::move(summary));
            }
        }

        results = std::move(parsedResults);
        error.clear();
        return true;
    } catch (...) {
        error = "test_results_parse_failed";
        return false;
    }
}

bool parseExamResultJson(const std::string& text, EnglishWordsExamResult& result, std::string& error) {
    try {
        auto root = LFJSONParser::parse(text);
        if (!root) {
            error = "test_result_parse_failed";
            return false;
        }

        result.resultId = stringOrEmpty(root, "resultId");
        result.fileName = stringOrEmpty(root, "fileName");
        result.createdAt = stringOrEmpty(root, "createdAt");
        result.levelId = stringOrEmpty(root, "levelId");
        result.levelTitle = stringOrEmpty(root, "levelTitle");
        result.mode = static_cast<EnglishWordsTestMode>(intField(root, "mode"));
        result.totalScore = numberField(root, "totalScore");
        result.questionCount = intField(root, "questionCount");

        if (root->contains("topic") && root->at("topic").isObject()) {
            auto topicObject = root->at("topic").asObject();
            result.topic.id = stringOrEmpty(topicObject, "id");
            result.topic.title = stringOrEmpty(topicObject, "title");
            result.topic.fileAssetPath = stringOrEmpty(topicObject, "file");
        }

        result.questions.clear();
        if (root->contains("questions") && root->at("questions").isArray()) {
            for (auto& value : root->at("questions").asArray()) {
                if (!value.isObject()) {
                    continue;
                }

                auto object = value.asObject();
                EnglishWordsQuestionResult question;
                question.mode = static_cast<EnglishWordsTestMode>(intField(object, "mode"));
                question.userAnswer = stringOrEmpty(object, "userAnswer");
                question.score = numberField(object, "score");

                if (object->contains("entry") && object->at("entry").isObject()) {
                    question.entry = parseWordEntryObject(object->at("entry").asObject());
                }

                result.questions.push_back(std::move(question));
            }
        }

        if (result.questionCount <= 0) {
            result.questionCount = static_cast<int>(result.questions.size());
        }

        error.clear();
        return true;
    } catch (...) {
        error = "test_result_parse_failed";
        return false;
    }
}

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return "";
    }

    return std::string(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>()
    );
}

bool writeTextFile(const std::filesystem::path& path, const std::string& content) {
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return false;
        }
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << content;
    return output.good();
}

std::filesystem::path resultsStorageRootPath(const std::string& applicationSupportDirectory) {
    return std::filesystem::u8path(applicationSupportDirectory) / "english_words";
}

std::filesystem::path resultsIndexFilePath(const std::string& applicationSupportDirectory) {
    return resultsStorageRootPath(applicationSupportDirectory) / "test_results.json";
}

std::filesystem::path resultFilePath(const std::string& applicationSupportDirectory, const std::string& fileName) {
    return resultsStorageRootPath(applicationSupportDirectory) / "results" / fileName;
}

} // namespace

EnglishWordsDataManager::Ptr EnglishWordsDataManager::create() {
    return std::make_shared<EnglishWordsDataManager>();
}

void EnglishWordsDataManager::preloadLevels() {
    auto& cache = sharedLevelsCache();
    if (cache.state == SharedLevelsCacheState::Ready ||
        cache.state == SharedLevelsCacheState::Loading) {
        return;
    }

    beginSharedLevelsLoad();
}

void EnglishWordsDataManager::loadLevels(LoadLevelsCallback callback) {
    if (!callback) {
        return;
    }

    auto& cache = sharedLevelsCache();
    if (cache.state == SharedLevelsCacheState::Ready) {
        callback(true, cache.levels, "");
        return;
    }

    cache.pendingCallbacks.push_back(std::move(callback));
    beginSharedLevelsLoad();
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

void EnglishWordsDataManager::saveExamResult(EnglishWordsExamResult result, SaveExamResultCallback callback) {
    if (!callback) {
        return;
    }

    prepareExamResultForSave(result);

    auto manager = shared_from_this();
    resolveApplicationSupportDirectory(
        [manager, result = std::move(result), callback = std::move(callback)](const std::string& directory) mutable {
            (void)manager;
            if (directory.empty()) {
                callback(false, std::move(result), "app_support_path_unavailable");
                return;
            }

            std::vector<EnglishWordsSavedResultSummary> savedResults;
            const auto indexPath = resultsIndexFilePath(directory);
            const std::string indexContent = readTextFile(indexPath);
            if (!indexContent.empty()) {
                std::string parseError;
                if (!parseSavedResultsIndexJson(indexContent, savedResults, parseError)) {
                    callback(false, std::move(result), parseError.empty() ? "test_results_parse_failed" : parseError);
                    return;
                }
            }

            if (!writeTextFile(resultFilePath(directory, result.fileName), serializeExamResultJson(result))) {
                callback(false, std::move(result), "test_result_write_failed");
                return;
            }

            std::vector<EnglishWordsSavedResultSummary> updatedResults;
            updatedResults.reserve(savedResults.size() + 1);
            updatedResults.push_back(buildSavedResultSummary(result));
            for (const auto& summary : savedResults) {
                if (summary.fileName != result.fileName) {
                    updatedResults.push_back(summary);
                }
            }

            if (!writeTextFile(indexPath, serializeSavedResultsIndexJson(updatedResults))) {
                callback(false, std::move(result), "test_results_write_failed");
                return;
            }

            callback(true, std::move(result), "");
        }
    );
}

void EnglishWordsDataManager::loadSavedResults(LoadSavedResultsCallback callback) {
    if (!callback) {
        return;
    }

    auto manager = shared_from_this();
    resolveApplicationSupportDirectory(
        [manager, callback = std::move(callback)](const std::string& directory) mutable {
            (void)manager;
            if (directory.empty()) {
                callback(false, {}, "app_support_path_unavailable");
                return;
            }

            const auto indexPath = resultsIndexFilePath(directory);
            const std::string content = readTextFile(indexPath);
            if (content.empty()) {
                callback(true, {}, "");
                return;
            }

            std::vector<EnglishWordsSavedResultSummary> results;
            std::string parseError;
            if (!parseSavedResultsIndexJson(content, results, parseError)) {
                callback(false, {}, parseError.empty() ? "test_results_parse_failed" : parseError);
                return;
            }

            callback(true, std::move(results), "");
        }
    );
}

void EnglishWordsDataManager::loadExamResult(const std::string& fileName, LoadExamResultCallback callback) {
    if (!callback) {
        return;
    }
    if (fileName.empty()) {
        callback(false, {}, "result_file_missing");
        return;
    }

    auto manager = shared_from_this();
    resolveApplicationSupportDirectory(
        [manager, fileName, callback = std::move(callback)](const std::string& directory) mutable {
            (void)manager;
            if (directory.empty()) {
                callback(false, {}, "app_support_path_unavailable");
                return;
            }

            const std::string content = readTextFile(resultFilePath(directory, fileName));
            if (content.empty()) {
                callback(false, {}, "test_result_load_failed");
                return;
            }

            EnglishWordsExamResult result;
            std::string parseError;
            if (!parseExamResultJson(content, result, parseError)) {
                callback(false, {}, parseError.empty() ? "test_result_parse_failed" : parseError);
                return;
            }

            callback(true, std::move(result), "");
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

void EnglishWordsDataManager::resolveApplicationSupportDirectory(DirectoryCallback callback) {
    if (!callback) {
        return;
    }

    if (!m_applicationSupportDirectory.empty()) {
        callback(m_applicationSupportDirectory);
        return;
    }

    m_pendingApplicationSupportDirectoryCallbacks.push_back(std::move(callback));
    if (m_resolvingApplicationSupportDirectory) {
        return;
    }

    m_resolvingApplicationSupportDirectory = true;
    std::weak_ptr<EnglishWordsDataManager> weakSelf = weak_from_this();
    LFPathProvider::getApplicationSupportPath([weakSelf](const LFPathProviderResult& result) {
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }

        self->m_resolvingApplicationSupportDirectory = false;
        self->m_applicationSupportDirectory = result.ok ? result.path : "";

        auto callbacks = std::move(self->m_pendingApplicationSupportDirectoryCallbacks);
        self->m_pendingApplicationSupportDirectoryCallbacks.clear();
        for (auto& current : callbacks) {
            if (current) {
                current(self->m_applicationSupportDirectory);
            }
        }
    });
}
