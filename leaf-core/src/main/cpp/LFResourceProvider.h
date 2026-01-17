//
// Created by Chen Tong on 2026/1/18.
//

#ifndef LEAF_LFRESOURCEPROVIDER_H
#define LEAF_LFRESOURCEPROVIDER_H

#include <string>
#include <functional>
#include <vector>

struct LFImageData {
    unsigned char *data;
    size_t size;
    int width;
    int height;

    ~LFImageData() {
        if (data) {
            free(data);
        }
    }
};

using LFImageLoader = std::function<void(const std::string&, std::function<void(std::shared_ptr<LFImageData>)>)>;

class LFResourceProvider {
public:
    static LFResourceProvider& getInstance() {
        static LFResourceProvider instance;
        return instance;
    }

    void setImageLoader(LFImageLoader loader) { m_nativeLoader = loader; }

    void fetchImageBuffer(const std::string& uri, std::function<void(std::shared_ptr<LFImageData>)> callback) {
        if (m_nativeLoader) {
            m_nativeLoader(uri, callback);
        }
    }

private:
    LFResourceProvider() = default;
    LFImageLoader m_nativeLoader;
};

#endif
