//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFRESOURCEPROVIDER_H
#define LEAF_LFRESOURCEPROVIDER_H

#include <string>
#include <functional>
#include <vector>

// TODO: 为以后扩展参数做准备，目前没有这个需求
using LFVariant = std::variant<int, float, std::string, std::vector<unsigned char>>;

struct LFData {
    unsigned char *data;
    size_t size;

    ~LFData() {
        if (data) {
            free(data);
        }
    }
};

using LFAssetLoader = std::function<void(const std::string&, std::function<void(std::shared_ptr<LFData>)>)>;

class LFResourceProvider {
public:
    static LFResourceProvider& getInstance() {
        static LFResourceProvider instance;
        return instance;
    }

    void setAssetLoader(LFAssetLoader loader) { m_nativeLoader = loader; }

    void fetchAsset(const std::string& uri, std::function<void(std::shared_ptr<LFData>)> callback) {
        if (m_nativeLoader) {
            m_nativeLoader(uri, callback);
        }
    }

private:
    LFResourceProvider() = default;
    LFAssetLoader m_nativeLoader;
};

#endif
