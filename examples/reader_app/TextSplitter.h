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

struct SplitIterator {
    const char* currentPos;
    const char* endPos;
    bool isFinished = false;
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

    /**
     * 单步分片切分
     * @param iter 迭代器，保存当前切分进度
     * @param batchSize 本次迭代切出的最大页数
     * @return 本次切出的页面列表
     */
    static std::vector<std::string> splitStep(SplitIterator& iter,
                                              const SplitConfig& config,
                                              int batchSize = 10);

private:
    // 禁止实例化，纯工具类
    TextSplitter() = default;
};

#endif //TEXTSPLITTER_H
