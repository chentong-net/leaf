//
// Created by Chen Tong on 2026/2/22.
//

#include "BookContentLoader.h"

#include <fstream>
#include <sstream>

BookContentLoadResult BookContentLoader::loadTextFile(const std::string& filePath) {
    BookContentLoadResult result;
    if (filePath.empty()) {
        result.error = "empty_path";
        return result;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        result.error = "open_file_failed";
        return result;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    if (!file.good() && !file.eof()) {
        result.error = "read_file_failed";
        return result;
    }

    result.ok = true;
    result.content = oss.str();
    return result;
}
