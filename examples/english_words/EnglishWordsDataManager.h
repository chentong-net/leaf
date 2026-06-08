//
// EnglishWords data access and asset path mapping.
//

#ifndef ENGLISHWORDS_DATA_MANAGER_H
#define ENGLISHWORDS_DATA_MANAGER_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct EnglishWordTopic {
    std::string id;
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
    std::string russianTranslation;
    std::string chineseTranslation;
    std::string audioAssetPath;
};

enum class EnglishWordsTestMode {
    ChineseToEnglish = 0,
    RussianToEnglish,
    EnglishToChinese,
    EnglishToRussian,
    AudioToChinese,
    AudioToRussian,
    AudioToEnglish,
};

struct EnglishWordsQuestionResult {
    EnglishWordEntry entry;
    EnglishWordsTestMode mode = EnglishWordsTestMode::AudioToEnglish;
    std::string userAnswer;
    double score = 0.0;
};

struct EnglishWordsExamResult {
    std::string resultId;
    std::string fileName;
    std::string createdAt;
    std::string levelId;
    std::string levelTitle;
    EnglishWordTopic topic;
    EnglishWordsTestMode mode = EnglishWordsTestMode::AudioToEnglish;
    double totalScore = 0.0;
    int questionCount = 0;
    std::vector<EnglishWordsQuestionResult> questions;
};

struct EnglishWordsSavedResultSummary {
    std::string resultId;
    std::string fileName;
    std::string createdAt;
    std::string levelId;
    std::string levelTitle;
    EnglishWordTopic topic;
    EnglishWordsTestMode mode = EnglishWordsTestMode::AudioToEnglish;
    double totalScore = 0.0;
    int questionCount = 0;
};

class EnglishWordsDataManager : public std::enable_shared_from_this<EnglishWordsDataManager> {
public:
    using Ptr = std::shared_ptr<EnglishWordsDataManager>;
    using LoadLevelsCallback = std::function<void(bool ok, std::vector<EnglishWordLevel> levels, const std::string& error)>;
    using LoadEntriesCallback = std::function<void(bool ok, std::vector<EnglishWordEntry> entries, const std::string& error)>;
    using ResolveAudioPathCallback = std::function<void(bool ok, std::string path, const std::string& error)>;
    using SaveExamResultCallback = std::function<void(bool ok, EnglishWordsExamResult result, const std::string& error)>;
    using LoadSavedResultsCallback = std::function<void(bool ok, std::vector<EnglishWordsSavedResultSummary> results, const std::string& error)>;
    using LoadExamResultCallback = std::function<void(bool ok, EnglishWordsExamResult result, const std::string& error)>;
    using DeleteSavedResultsCallback = std::function<void(bool ok, const std::string& error)>;

    static Ptr create();
    static void preloadLevels();

    void loadLevels(LoadLevelsCallback callback);
    void loadEntries(const EnglishWordTopic& topic, LoadEntriesCallback callback);
    void resolveAudioPath(const std::string& audioAssetPath, ResolveAudioPathCallback callback);
    void saveExamResult(EnglishWordsExamResult result, SaveExamResultCallback callback);
    void loadSavedResults(LoadSavedResultsCallback callback);
    void loadExamResult(const std::string& fileName, LoadExamResultCallback callback);
    void deleteSavedResults(const std::vector<std::string>& resultIds, DeleteSavedResultsCallback callback);

private:
    using DirectoryCallback = std::function<void(const std::string&)>;

    void resolveTempDirectory(DirectoryCallback callback);
    void resolveApplicationSupportDirectory(DirectoryCallback callback);

    std::string m_tempDirectory;
    bool m_resolvingTempDirectory = false;
    std::vector<DirectoryCallback> m_pendingDirectoryCallbacks;
    std::string m_applicationSupportDirectory;
    bool m_resolvingApplicationSupportDirectory = false;
    std::vector<DirectoryCallback> m_pendingApplicationSupportDirectoryCallbacks;
    std::unordered_map<std::string, std::string> m_cachedAudioPaths;
};

#endif // ENGLISHWORDS_DATA_MANAGER_H
