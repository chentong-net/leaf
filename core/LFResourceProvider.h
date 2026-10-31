//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFRESOURCEPROVIDER_H
#define LEAF_LFRESOURCEPROVIDER_H

#include <string>
#include <functional>
#include <vector>
#include <mutex>
#include <utility>

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

    void setAssetLoader(LFAssetLoader loader) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_nativeLoader = std::move(loader);
    }

    void fetchAsset(const std::string& uri, std::function<void(std::shared_ptr<LFData>)> callback) {
        LFAssetLoader loader;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            loader = m_nativeLoader;
        }
        if (!loader) {
            if (callback) {
                callback(nullptr);
            }
            return;
        }
        loader(uri, callback);
    }

private:
    LFResourceProvider() = default;
    std::mutex m_mutex;
    LFAssetLoader m_nativeLoader;
};

#endif
