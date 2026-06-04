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
    std::string audioAssetPath;
};

class EnglishWordsDataManager : public std::enable_shared_from_this<EnglishWordsDataManager> {
public:
    using Ptr = std::shared_ptr<EnglishWordsDataManager>;
    using LoadLevelsCallback = std::function<void(bool ok, std::vector<EnglishWordLevel> levels, const std::string& error)>;
    using LoadEntriesCallback = std::function<void(bool ok, std::vector<EnglishWordEntry> entries, const std::string& error)>;
    using ResolveAudioPathCallback = std::function<void(bool ok, std::string path, const std::string& error)>;

    static Ptr create();

    void loadLevels(LoadLevelsCallback callback);
    void loadEntries(const EnglishWordTopic& topic, LoadEntriesCallback callback);
    void resolveAudioPath(const std::string& audioAssetPath, ResolveAudioPathCallback callback);

private:
    using DirectoryCallback = std::function<void(const std::string&)>;

    void resolveTempDirectory(DirectoryCallback callback);

    std::string m_tempDirectory;
    bool m_resolvingTempDirectory = false;
    std::vector<DirectoryCallback> m_pendingDirectoryCallbacks;
    std::unordered_map<std::string, std::string> m_cachedAudioPaths;
};

#endif // ENGLISHWORDS_DATA_MANAGER_H
