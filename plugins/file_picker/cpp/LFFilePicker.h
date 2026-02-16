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

struct LFFilePickerResult {
    bool ok = false;
    bool canceled = false;
    std::string path;
    std::string error;
};

using LFFilePickerCallback = std::function<void(const LFFilePickerResult&)>;

class LFFilePicker {
public:
    static void pickFile(LFFilePickerCallback callback);
    static void pickFile(const LFFilePickerOptions& options, LFFilePickerCallback callback);
};

// Platform bridge callback: platform implementation -> C++ plugin core
extern "C" void lfFilePickerOnPlatformResult(
        int requestId,
        int success,
        const char* path,
        const char* error);

#endif // LEAF_LFFILEPICKER_H
