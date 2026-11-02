//
// Created by Chen Tong on 2026/2/2.
//

#ifndef APPUTILS_H
#define APPUTILS_H

#include "LFEngine.h"

namespace AppUtils {

    // 使用 inline 允许在头文件中定义实现
    inline std::shared_ptr<LFText> createLabel(const std::string& text, float size, uint32_t color, bool bold = false) {
        auto t = std::make_shared<LFText>();
        t->setText(text);
        t->setFontSize(size);
        t->setTextColor(color);
        t->setTextHAlign(LFTextHAlign::Center);
        t->setTextVAlign(LFTextVAlign::Center);
        if (bold) {
            t->setFontWeight(LFFontWeight::Bold);
        }
        return t;
    }

}

#endif // APPUTILS_H
