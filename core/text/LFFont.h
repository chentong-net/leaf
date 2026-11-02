#ifndef LEAF_LFFONT_H
#define LEAF_LFFONT_H

/**
 * 内置字体的字重层级。
 * 自定义字体族仍可通过 LFText::setFontFamily 指定。
 */
enum class LFFontWeight {
    Regular,
    Medium,
    SemiBold,
    Bold
};

struct LFBundledFont {
    LFFontWeight weight;
    const char* fileName;
    const char* familyName;
};

inline constexpr const char* kLFDefaultFontFamily = "sans";

inline constexpr LFBundledFont kLFBundledFonts[] = {
    {LFFontWeight::Regular, "Alibaba-PuHuiTi-Regular.ttf", "sans"},
    {LFFontWeight::Medium, "AlibabaPuHuiTi-3-65-Medium.ttf", "sans-medium"},
    {LFFontWeight::SemiBold, "AlibabaPuHuiTi-3-75-SemiBold.ttf", "sans-semibold"},
    {LFFontWeight::Bold, "AlibabaPuHuiTi-3-85-Bold.ttf", "sans-bold"}
};

inline constexpr const char* fontFamilyForWeight(LFFontWeight weight) noexcept {
    switch (weight) {
        case LFFontWeight::Medium:
            return "sans-medium";
        case LFFontWeight::SemiBold:
            return "sans-semibold";
        case LFFontWeight::Bold:
            return "sans-bold";
        case LFFontWeight::Regular:
        default:
            return kLFDefaultFontFamily;
    }
}

#endif // LEAF_LFFONT_H
