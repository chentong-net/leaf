//
// Created by Chen Tong on 2026/1/31.
// 专门用于长文本分页计算的业务工具类
//

#ifndef TEXTSPLITTER_H
#define TEXTSPLITTER_H

#include <string>
#include <vector>
#include "LFEngine.h" // 需要引用引擎以获取 NanoVG 上下文

/**
 * 分页配置参数
 */
struct SplitConfig {
    float width;            // 内容区域宽度 (px)
    float height;           // 内容区域高度 (px)
    float fontSize;         // 字体大小 (px)
    float lineHeight = 1.6f;// 行高倍数 (默认 1.6 倍)
    std::string fontFamily = "sans"; // 字体名称
};

class TextSplitter {
public:
    /**
     * 核心分页算法
     * 将长字符串切割成适合屏幕显示的页面列表
     *
     * @param fullText 整本书或整章的内容
     * @param config   排版配置
     * @return std::vector<std::string> 每一页的文本内容
     */
    static std::vector<std::string> split(const std::string& fullText, const SplitConfig& config);

private:
    // 禁止实例化，纯工具类
    TextSplitter() = default;
};

#endif //TEXTSPLITTER_H
