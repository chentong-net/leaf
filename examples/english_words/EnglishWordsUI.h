//
// Shared UI helpers for EnglishWords pages.
//

#ifndef ENGLISHWORDS_UI_H
#define ENGLISHWORDS_UI_H

#include "LFEngine.h"

#include <functional>
#include <string>

namespace EnglishWordsUI {

inline constexpr uint32_t kPageBackgroundColor = 0xFFF4F7FB;
inline constexpr uint32_t kTitleColor = 0xFF142033;
inline constexpr uint32_t kSurfaceColor = 0xFFEAF1F8;
inline constexpr uint32_t kSurfaceBorderColor = 0xFFDDE7F1;
inline constexpr uint32_t kCardBorderColor = 0xFFDCE6F2;
inline constexpr uint32_t kCardShadowColor = 0x12233B53;

inline std::shared_ptr<LFText> makeText(const std::string& text, float size, uint32_t color) {
    auto node = std::make_shared<LFText>();
    node->setText(text);
    node->setFontSize(size);
    node->setTextColor(color);
    node->setLineHeight(1.3f);
    return node;
}

inline std::shared_ptr<LFImage> makeImage(const std::string& src, float size) {
    auto image = std::make_shared<LFImage>();
    image->setWidth(size);
    image->setHeight(size);
    image->setFit(LFImageFit::Contain);
    image->setSrc(src);
    return image;
}

inline std::shared_ptr<LFLinear> makeHeaderButton(const std::string& iconPath, std::function<void()> onTap) {
    auto surface = LFLinear::createHorizontal();
    surface->setWidth(46.0f);
    surface->setHeight(46.0f);
    surface->setBorderRadius(16.0f);
    surface->setBorder(1.0f, 0xFFD8E4F1);
    surface->setBackgroundColor(0xFFFFFFFF);
    surface->setShadow(0.0f, 6.0f, 18.0f, 0.0f, 0x10233B53);
    surface->setGravity(LFAlignment::Center, LFAlignment::Center);
    surface->addChild(makeImage(iconPath, 18.0f));
    surface->setOnTap([onTap = std::move(onTap)](const LFPoint&) {
        if (onTap) {
            onTap();
        }
    });
    return surface;
}

inline std::shared_ptr<LFLinear> createPageRoot(float bottomPadding = 0.0f) {
    auto root = LFLinear::createVertical();
    root->matchParentWidth();
    root->matchParentHeight();
    root->setPadding(YGEdgeTop, 20.0f);
    root->setPadding(YGEdgeLeft, 20.0f);
    root->setPadding(YGEdgeRight, 20.0f);
    if (bottomPadding > 0.0f) {
        root->setPadding(YGEdgeBottom, bottomPadding);
    }
    root->setSpacing(16.0f);
    return root;
}

inline void addPageHeader(const std::shared_ptr<LFLinear>& root,
                          const std::string& titleText,
                          std::function<void()> onBack,
                          float titleSize) {
    if (!root) {
        return;
    }

    auto headerRow = LFLinear::createHorizontal();
    headerRow->matchParentWidth();
    headerRow->wrapContentHeight();
    headerRow->setAlignItems(YGAlignCenter);
    headerRow->setSpacing(12.0f);

    headerRow->addChild(makeHeaderButton("EnglishWordsAssets/Images/icon-arrow-left.png", std::move(onBack)));

    auto titleWrap = LFLinear::createVertical();
    titleWrap->setFlexGrow(1.0f);
    titleWrap->setFlexBasis(0.0f);
    titleWrap->wrapContentHeight();

    auto title = makeText(titleText, titleSize, kTitleColor);
    title->matchParentWidth();
    title->setTextHAlign(LFTextHAlign::Center);
    title->setMaxLines(1);
    titleWrap->addChild(title);
    headerRow->addChild(titleWrap);

    auto spacer = LFBox::create();
    spacer->setWidth(46.0f);
    spacer->setHeight(46.0f);
    headerRow->addChild(spacer);

    root->addChild(headerRow);
}

inline void clearChildren(const LFNode::Ptr& node) {
    if (!node) {
        return;
    }

    auto children = node->getChildren();
    for (const auto& child : children) {
        node->removeChild(child);
    }
}

inline std::shared_ptr<LFLinear> makeStatusCard(const std::string& text) {
    auto card = LFLinear::createVertical();
    card->matchParentWidth();
    card->wrapContentHeight();
    card->setPadding(YGEdgeAll, 24.0f);
    card->setBorderRadius(22.0f);
    card->setBackgroundColor(0xFFFFFFFF);
    card->setBorder(1.0f, kCardBorderColor);
    card->setShadow(0.0f, 8.0f, 22.0f, 0.0f, kCardShadowColor);

    auto message = makeText(text, 14.0f, 0xFF526275);
    message->matchParentWidth();
    message->setTextHAlign(LFTextHAlign::Center);
    card->addChild(message);

    return card;
}

} // namespace EnglishWordsUI

#endif // ENGLISHWORDS_UI_H
