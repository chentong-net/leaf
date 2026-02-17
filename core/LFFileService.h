#ifndef LEAF_LFFILESERVICE_H
#define LEAF_LFFILESERVICE_H

#include "LFDef.h"

enum class LFFileMediaType {
    Any = 0,
    Image = 1,
    Video = 2,
    ImageOrVideo = 3
};

struct LFFilePickOptions {
    LFFileMediaType mediaType = LFFileMediaType::Any;
    bool copyToSandbox = false;
};

struct LFFileDescriptor {
    std::string fileId;
    std::string name;
    std::string mimeType;
    int64_t size = 0;
    std::string path;
    bool hasLocalPath = false;
};

struct LFFilePickResult {
    bool ok = false;
    bool canceled = false;
    std::vector<LFFileDescriptor> files;
    std::string error;
};

struct LFFileReadResult {
    bool ok = false;
    bool canceled = false;
    std::string content;
    std::string error;
};

struct LFFileSaveOptions {
    std::string fileName;
};

struct LFFileSaveResult {
    bool ok = false;
    bool canceled = false;
    std::string path;
    std::string error;
};

using LFFilePickCallback = std::function<void(const LFFilePickResult&)>;
using LFFileReadCallback = std::function<void(const LFFileReadResult&)>;
using LFFileSaveCallback = std::function<void(const LFFileSaveResult&)>;

class LFFileService {
public:
    virtual ~LFFileService() = default;

    virtual void pickFile(const LFFilePickOptions& options, LFFilePickCallback callback) = 0;
    virtual void readFile(const std::string& fileId, LFFileReadCallback callback) = 0;
    virtual void saveFile(const LFFileSaveOptions& options, const std::string& content, LFFileSaveCallback callback) = 0;
};

class LFFileSystem {
public:
    static void setFileService(const std::shared_ptr<LFFileService>& service);

    static void pickFile(const LFFilePickOptions& options, LFFilePickCallback callback);
    static void readFile(const std::string& fileId, LFFileReadCallback callback);
    static void saveFile(const LFFileSaveOptions& options, const std::string& content, LFFileSaveCallback callback);
};

#endif // LEAF_LFFILESERVICE_H
