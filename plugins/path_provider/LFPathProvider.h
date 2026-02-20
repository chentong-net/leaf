//
// Created by Chen Tong on 2026/2/20.
//

#ifndef LEAF_LFPATHPROVIDER_H
#define LEAF_LFPATHPROVIDER_H

#include "LFDef.h"

struct LFPathProviderResult {
    bool ok = false;
    std::string path;
    std::string error;
};

using LFPathProviderCallback = std::function<void(const LFPathProviderResult&)>;

class LFPathProvider {
public:
    static void getTemporaryPath(LFPathProviderCallback callback);
    static void getApplicationSupportPath(LFPathProviderCallback callback);
    static void getApplicationDocumentsPath(LFPathProviderCallback callback);
    static void getDownloadsPath(LFPathProviderCallback callback);
    static void getExternalStoragePath(LFPathProviderCallback callback);
};

#endif // LEAF_LFPATHPROVIDER_H
