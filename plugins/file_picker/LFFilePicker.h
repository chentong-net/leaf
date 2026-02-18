//
// Created by Chen Tong on 2026/2/18.
//

#ifndef LEAF_LFFILEPICKER_H
#define LEAF_LFFILEPICKER_H

#include "LFDef.h"

enum class LFFilePickerMediaType {
    Any = 0,
    Image = 1,
    Video = 2,
    ImageOrVideo = 3
};

struct LFFilePickerOptions {
    LFFilePickerMediaType mediaType = LFFilePickerMediaType::Any;
    bool copyToSandbox = true;
};

struct LFFileInfo {
    std::string fileId;
    std::string path;
    std::string name;
    std::string mimeType;
    int64_t size = 0;
};

struct LFFilePickResult {
    bool ok = false;
    bool canceled = false;
    LFFileInfo file;
    std::string error;
};

using LFFilePickCallback = std::function<void(const LFFilePickResult&)>;

struct LFFileReadOptions {
    size_t offset = 0;
    size_t length = 256 * 1024;
};

struct LFFileReadResult {
    bool ok = false;
    bool eof = false;
    std::vector<unsigned char> bytes;
    std::string error;
};

using LFFileReadCallback = std::function<void(const LFFileReadResult&)>;

class LFFilePicker {
public:
    static void pickFile(LFFilePickCallback callback);
    static void pickFile(const LFFilePickerOptions& options, LFFilePickCallback callback);

    static void readFile(const LFFileInfo& file, LFFileReadCallback callback);
    static void readFile(const LFFileInfo& file, const LFFileReadOptions& options, LFFileReadCallback callback);
};

#endif // LEAF_LFFILEPICKER_H
