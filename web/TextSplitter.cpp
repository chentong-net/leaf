//
// Created by Chen Tong on 2026/1/31.
//

#include "TextSplitter.h"
#include <cmath>
#include <algorithm>

std::vector<std::string> TextSplitter::split(const std::string& fullText, const SplitConfig& config) {
    std::vector<std::string> pages;

    if (fullText.empty()) return pages;
    if (config.width <= 0 || config.height <= 0) return pages;

    NVGcontext* ctx = LFEngine::getInstance().getNVGContext();
    if (!ctx) return pages;

    nvgSave(ctx);

    // 1. 严格统一字体配置
    nvgFontSize(ctx, config.fontSize);
    nvgFontFace(ctx, config.fontFamily.c_str());
    nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    // 关键点：虽然设置了 nvgTextLineHeight，但我们在计算时手动控制 y 步进更安全
    nvgTextLineHeight(ctx, config.lineHeight);

    // 2. 强制定义行高 (The Golden Rule)
    // 渲染器 LFText 最终渲染时的行间距就是 fontSize * lineHeight
    // 所以 Splitter 必须严格遵守这个值，差 1px 都不行。
    float rowHeight = config.fontSize * config.lineHeight;

    // 防止死循环
    if (rowHeight < 1.0f) rowHeight = config.fontSize;

    const char* start = fullText.c_str();
    const char* end = start + fullText.size();
    const char* current = start;

    pages.reserve(fullText.size() / 200 + 1);

    while (current < end) {
        const char* pageStart = current;
        float currentY = 0.0f; // 当前页已用的高度
        int lineCount = 0;

        // --- 核心排版循环 ---
        while (current < end) {
            // 预测下一行底部的坐标
            // 使用 > 而不是 >=，留出微小的浮点数容差
            // 逻辑：如果放了这一行，bottom 会超过 height，那就不能放
            if (currentY + rowHeight > config.height) {
                // 特殊情况：如果这是这一页的第一行，无论如何也要放进去，否则死循环
                if (lineCount > 0) {
                    break; // 空间满了，换页
                }
            }

            // 测量这一行能放多少字
            NVGtextRow row;
            int count = nvgTextBreakLines(ctx, current, end, config.width, &row, 1);

            if (count == 0) {
                // 异常：一个字都放不下 (可能是宽度太窄或遇到非法字符)
                if (current < end) current++;
                else break;
            } else {
                // 成功放入一行
                currentY += rowHeight; // 严格累加固定行高
                lineCount++;
                current = row.next;    // 移动指针
            }
        }

        // 截取页面
        if (current > pageStart) {
            pages.emplace_back(pageStart, current - pageStart);
        } else {
            break;
        }
    }

    nvgRestore(ctx);
    return pages;
}

std::vector<std::string> TextSplitter::splitStep(SplitIterator& iter, const SplitConfig& config, int batchSize) {
    std::vector<std::string> pages;
    if (iter.isFinished || !iter.currentPos || iter.currentPos >= iter.endPos) {
        iter.isFinished = true;
        return pages;
    }

    NVGcontext* ctx = LFEngine::getInstance().getNVGContext();
    nvgSave(ctx);
    nvgFontSize(ctx, config.fontSize);
    nvgFontFace(ctx, config.fontFamily.c_str());
    nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgTextLineHeight(ctx, config.lineHeight);

    float rowHeight = config.fontSize * config.lineHeight;

    while (iter.currentPos < iter.endPos && pages.size() < batchSize) {
        nvgFontSize(ctx, config.fontSize);
        nvgFontFace(ctx, config.fontFamily.c_str());
        const char* pageStart = iter.currentPos;
        float currentY = 0.0f;
        int lineCount = 0;

        while (iter.currentPos < iter.endPos) {
            if (currentY + rowHeight > config.height) {
                if (lineCount > 0) break;
            }

            NVGtextRow row;
            int count = nvgTextBreakLines(ctx, iter.currentPos, iter.endPos, config.width, &row, 1);
            if (count == 0) {
                if (iter.currentPos < iter.endPos) iter.currentPos++;
                else break;
            } else {
                currentY += rowHeight;
                lineCount++;
                iter.currentPos = row.next;
            }
        }

        if (iter.currentPos > pageStart) {
            pages.emplace_back(pageStart, iter.currentPos - pageStart);
        }
    }

    if (iter.currentPos >= iter.endPos) {
        iter.isFinished = true;
    }

    nvgRestore(ctx);
    return pages;
}
