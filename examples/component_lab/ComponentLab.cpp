#include "ComponentLab.h"
#include "LFEngine.h"
#include "ProfilePage.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

std::shared_ptr<LFText> makeText(const std::string& text, float size, uint32_t color) {
    auto node = std::make_shared<LFText>();
    node->setText(text);
    node->setFontSize(size);
    node->setTextColor(color);
    node->setLineHeight(1.35f);
    return node;
}

LFButton::Ptr makeActionButton(const std::string& text, LFButton::ClickCallback onClick) {
    auto button = LFButton::create(text, std::move(onClick));
    button->setHeight(40.0f);
    button->setBorderRadius(10.0f);
    button->setFontSize(14.0f);
    button->setTextColor(0xFF1E3A5F);
    button->setBackgroundColor(LFButtonState::Normal, 0xFFEAF3FF);
    button->setBackgroundColor(LFButtonState::Pressed, 0xFFD9E9FA);
    button->setBorder(1.0f, 0xFFC8DBF2);
    return button;
}

LFLinear::Ptr makeSectionCard(const std::string& titleText, const std::string& descText) {
    auto card = LFLinear::createVertical();
    card->matchParentWidth();
    card->wrapContentHeight();
    card->setPadding(YGEdgeAll, 18.0f);
    card->setSpacing(14.0f);
    card->setBackgroundColor(0xFFFFFFFF);
    card->setBorderRadius(18.0f);
    card->setBorder(1.0f, 0xFFE0E7F0);
    card->setShadow(0.0f, 8.0f, 24.0f, 0.0f, 0x14203040);

    auto title = makeText(titleText, 18.0f, 0xFF142033);
    title->matchParentWidth();
    card->addChild(title);

    auto desc = makeText(descText, 13.0f, 0xFF637083);
    desc->matchParentWidth();
    card->addChild(desc);

    return card;
}

LFLinear::Ptr makeRow() {
    auto row = LFLinear::createHorizontal();
    row->matchParentWidth();
    row->wrapContentHeight();
    row->setSpacing(10.0f);
    row->setAlignItems(YGAlignCenter);
    return row;
}

void addLabelValueRow(const LFLinear::Ptr& parent,
                      const std::string& labelText,
                      const LFNode::Ptr& control) {
    auto row = makeRow();

    auto label = makeText(labelText, 14.0f, 0xFF263547);
    label->setFlexGrow(1.0f);
    row->addChild(label);
    row->addChild(control);

    parent->addChild(row);
}

LFLinear::Ptr makeOverlayCard(const std::string& text) {
    auto card = LFLinear::createVertical();
    card->setWidth(100.0f);
    card->wrapContentHeight();
    card->setPadding(YGEdgeAll, 12.0f);
    card->setBackgroundColor(0xFFFFFFFF);
    card->setBorderRadius(12.0f);
    card->setBorder(1.0f, 0xFFE3E8F0);
    card->setShadow(0.0f, 8.0f, 20.0f, 0.0f, 0x22000000);

    auto title = makeText(text, 13.0f, 0xFF152033);
    title->setTextVAlign(LFTextVAlign::Center);
    title->setTextHAlign(LFTextHAlign::Center);
    title->matchParentWidth();
    card->addChild(title);

    return card;
}

void setTextFormat(const std::shared_ptr<LFText>& text, const char* format, float value) {
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), format, value);
    text->setText(buffer);
}

LFLinear::Ptr buildFunctionDemoPage() {
    auto overlay = LFOverlay::create();
    overlay->setVisible(false);
    overlay->setModal(true);
    overlay->setBarrierColor(0x66000000);
    overlay->setDismissOnBarrierTap(true);

    auto page = LFLinear::createVertical();
    page->matchParentWidth();
    page->wrapContentHeight();
    page->setBackgroundColor(0xFFF5F7FB);
    page->setPadding(YGEdgeAll, 24.0f);
    page->setSpacing(18.0f);

    auto title = makeText("Leaf Component Lab", 26.0f, 0xFF142033);
    title->setTextHAlign(LFTextHAlign::Center);
    title->matchParentWidth();
    page->addChild(title);

    auto description = makeText(
        "Use this page to validate LFDropdown, LFToggle, LFOverlay, and LFSlider across layout, state, disabled, and interaction cases.",
        14.0f,
        0xFF637083
    );
    description->matchParentWidth();
    page->addChild(description);

    auto dropdownCard = makeSectionCard(
        "LFDropdown",
        "Covers placeholder, selection callback, long scrollable option panel, disabled state, custom colors, and scale toggle."
    );

    auto dropdownStatus = makeText("Selection: none", 14.0f, 0xFF3567A3);
    dropdownStatus->matchParentWidth();
    dropdownCard->addChild(dropdownStatus);

    auto dropdown = LFDropdown::create({
        "English Words",
        "Review Mode",
        "Spelling Practice",
        "Listening Practice",
        "Wrong Answers",
        "Daily Challenge",
        "Unit 1",
        "Unit 2",
        "Unit 3",
        "Unit 4"
    });
    dropdown->setPlaceholder("Choose training mode");
    dropdown->setOptionHeight(38.0f);
    dropdown->setMaxPanelHeight(160.0f);
    dropdown->setTriggerBackgroundColor(0xFFFFFFFF, 0xFFEAF3FF);
    dropdown->setOptionBackgroundColor(0xFFFFFFFF, 0xFFF2F7FF, 0xFFE5F0FF);
    dropdown->setOnSelectionChanged([dropdownStatus](int index, const std::string& value) {
        dropdownStatus->setText("Selection: #" + std::to_string(index) + " " + value);
    });
    dropdownCard->addChild(dropdown);

    auto compactDropdown = LFDropdown::create({"Small", "Medium", "Large"});
    compactDropdown->setSelectedIndex(1, false);
    compactDropdown->setOptionHeight(34.0f);
    compactDropdown->setMaxPanelHeight(120.0f);
    compactDropdown->setFontSize(13.0f);
    compactDropdown->setCornerRadius(14.0f);
    compactDropdown->setBorderColor(0xFF8BB7E5);
    compactDropdown->setSelectedTextColor(0xFF2F855A);
    dropdownCard->addChild(compactDropdown);

    auto disabledDropdown = LFDropdown::create({"Disabled A", "Disabled B"});
    disabledDropdown->setPlaceholder("Disabled dropdown");
    disabledDropdown->setEnabled(false);
    dropdownCard->addChild(disabledDropdown);

    auto dropdownControls = makeRow();
    auto toggleScaleButton = makeActionButton("Toggle Scale", [dropdown, compactDropdown](LFButton*) {
        bool next = !dropdown->isScaleEnabled();
        dropdown->enableScale(next);
        compactDropdown->enableScale(next);
    });
    auto resetDropdownButton = makeActionButton("Reset Selection", [dropdown, compactDropdown, dropdownStatus](LFButton*) {
        dropdown->setSelectedIndex(-1);
        compactDropdown->setSelectedIndex(1, false);
        dropdownStatus->setText("Selection: none");
    });
    toggleScaleButton->setFlexGrow(1.0f);
    resetDropdownButton->setFlexGrow(1.0f);
    dropdownControls->addChild(toggleScaleButton);
    dropdownControls->addChild(resetDropdownButton);
    dropdownCard->addChild(dropdownControls);

    page->addChild(dropdownCard);

    auto popupDropdownCard = makeSectionCard(
        "LFDropdown Popup",
        "Tests popup display mode, root-level overlay attachment, barrier dismissal, and popup placement."
    );

    auto popupStatus = makeText("Popup selection: none", 14.0f, 0xFF7C3AED);
    popupStatus->matchParentWidth();
    popupDropdownCard->addChild(popupStatus);

    auto popupDropdown = LFDropdown::create({
        "Vocabulary Drill",
        "Spell Check",
        "Listen & Match",
        "Daily Review",
        "Wrong Answer Review",
        "Timed Challenge",
        "Unit A",
        "Unit B"
    });
    popupDropdown->setDisplayMode(LFDropdownDisplayMode::Popup);
    popupDropdown->setPlaceholder("Open popup dropdown");
    popupDropdown->setOptionHeight(38.0f);
    popupDropdown->setMaxPanelHeight(180.0f);
    popupDropdown->setTriggerBackgroundColor(0xFFFFFFFF, 0xFFF3EEFF);
    popupDropdown->setOptionBackgroundColor(0xFFFFFFFF, 0xFFF7F2FF, 0xFFE9DDFF);
    popupDropdown->setSelectedTextColor(0xFF6B21A8);
    popupDropdown->setBorderColor(0xFFD7C4F3);
    popupDropdown->setOnSelectionChanged([popupStatus](int index, const std::string& value) {
        popupStatus->setText("Popup selection: #" + std::to_string(index) + " " + value);
    });
    popupDropdownCard->addChild(popupDropdown);

    auto popupTip = makeText("Tap the field to open popup mode. Tap the barrier to dismiss.", 12.0f, 0xFF8B6FB8);
    popupTip->matchParentWidth();
    popupDropdownCard->addChild(popupTip);

    page->addChild(popupDropdownCard);

    auto inlinePairCard = makeSectionCard(
        "LFDropdown Inline Pair",
        "Two inline dropdowns placed on one row."
    );

    auto inlinePairRow = makeRow();
    inlinePairRow->setAlignItems(YGAlignFlexStart);

    auto leftColumn = LFLinear::createVertical();
    leftColumn->setFlexGrow(1.0f);
    leftColumn->wrapContentHeight();
    leftColumn->setSpacing(8.0f);
    auto leftLabel = makeText("Left inline", 13.0f, 0xFF4B5563);
    leftLabel->matchParentWidth();
    leftColumn->addChild(leftLabel);
    auto leftDropdown = LFDropdown::create({"Alpha", "Beta", "Gamma", "Delta"});
    leftDropdown->setPlaceholder("Left dropdown");
    leftDropdown->setOptionHeight(34.0f);
    leftDropdown->setMaxPanelHeight(120.0f);
    leftDropdown->setFontSize(13.0f);
    leftDropdown->setCornerRadius(14.0f);
    leftDropdown->setBorderColor(0xFFC7D2FE);
    leftDropdown->setSelectedIndex(0, false);
    leftColumn->addChild(leftDropdown);

    auto rightColumn = LFLinear::createVertical();
    rightColumn->setFlexGrow(1.0f);
    rightColumn->wrapContentHeight();
    rightColumn->setSpacing(8.0f);
    auto rightLabel = makeText("Right inline", 13.0f, 0xFF4B5563);
    rightLabel->matchParentWidth();
    rightColumn->addChild(rightLabel);
    auto rightDropdown = LFDropdown::create({"One", "Two", "Three", "Four"});
    rightDropdown->setPlaceholder("Right dropdown");
    rightDropdown->setOptionHeight(34.0f);
    rightDropdown->setMaxPanelHeight(120.0f);
    rightDropdown->setFontSize(13.0f);
    rightDropdown->setCornerRadius(14.0f);
    rightDropdown->setBorderColor(0xFFBAE6FD);
    rightDropdown->setSelectedIndex(1, false);
    rightColumn->addChild(rightDropdown);

    inlinePairRow->addChild(leftColumn);
    inlinePairRow->addChild(rightColumn);
    inlinePairCard->addChild(inlinePairRow);

    page->addChild(inlinePairCard);

    auto toggleCard = makeSectionCard(
        "LFToggle",
        "Covers checked callback, animation, disabled opacity, custom colors, thumb padding, and multiple sizes."
    );

    auto toggleStatus = makeText("Primary toggle: on", 14.0f, 0xFF3567A3);
    toggleStatus->matchParentWidth();
    toggleCard->addChild(toggleStatus);

    auto primaryToggle = LFToggle::create(true);
    primaryToggle->setTrackSize(58.0f, 34.0f);
    primaryToggle->setOnCheckedChanged([toggleStatus](bool checked) {
        toggleStatus->setText(checked ? "Primary toggle: on" : "Primary toggle: off");
    });
    addLabelValueRow(toggleCard, "Default animated toggle", primaryToggle);

    auto customToggle = LFToggle::create(false);
    customToggle->setTrackSize(72.0f, 36.0f);
    customToggle->setThumbPadding(5.0f);
    customToggle->setTrackColor(0xFFFFE4E6, 0xFF0EA5E9);
    customToggle->setThumbColor(0xFFFFFFFF, 0xFFFFFFFF);
    customToggle->setBorderColor(0xFFF43F5E, 0xFF0284C7);
    customToggle->setBorderWidth(2.0f);
    addLabelValueRow(toggleCard, "Wide custom toggle", customToggle);

    auto tinyToggle = LFToggle::create(true);
    tinyToggle->setTrackSize(40.0f, 24.0f);
    tinyToggle->setThumbPadding(2.0f);
    addLabelValueRow(toggleCard, "Compact toggle", tinyToggle);

    auto disabledToggle = LFToggle::create(true);
    disabledToggle->setTrackSize(58.0f, 34.0f);
    disabledToggle->setDisabledOpacity(0.32f);
    disabledToggle->setEnabled(false);
    addLabelValueRow(toggleCard, "Disabled toggle", disabledToggle);

    auto toggleControls = makeRow();
    auto setAllOn = makeActionButton("Set All On", [primaryToggle, customToggle, tinyToggle](LFButton*) {
        primaryToggle->setChecked(true);
        customToggle->setChecked(true);
        tinyToggle->setChecked(true);
    });
    auto setAllOff = makeActionButton("Set All Off", [primaryToggle, customToggle, tinyToggle](LFButton*) {
        primaryToggle->setChecked(false);
        customToggle->setChecked(false);
        tinyToggle->setChecked(false);
    });
    setAllOn->setFlexGrow(1.0f);
    setAllOff->setFlexGrow(1.0f);
    toggleControls->addChild(setAllOn);
    toggleControls->addChild(setAllOff);
    toggleCard->addChild(toggleControls);

    page->addChild(toggleCard);

    auto overlayCard = makeSectionCard(
        "LFOverlay",
        "Covers auto-attach to root, modal and non-modal behavior, barrier tap dismissal, and nine-grid alignment."
    );

    auto overlayStatus = makeText("Overlay state: hidden", 14.0f, 0xFF3567A3);
    overlayStatus->matchParentWidth();
    overlayCard->addChild(overlayStatus);

    overlay->setOnDismiss([overlayStatus]() {
        overlayStatus->setText("Overlay state: hidden");
    });

    auto modalToggle = LFToggle::create(true);
    modalToggle->setTrackSize(58.0f, 34.0f);
    modalToggle->setOnCheckedChanged([overlay](bool checked) {
        overlay->setModal(checked);
    });
    addLabelValueRow(overlayCard, "Modal barrier", modalToggle);

    auto dismissToggle = LFToggle::create(true);
    dismissToggle->setTrackSize(58.0f, 34.0f);
    dismissToggle->setOnCheckedChanged([overlay](bool checked) {
        overlay->setDismissOnBarrierTap(checked);
    });
    addLabelValueRow(overlayCard, "Tap barrier to dismiss", dismissToggle);

    page->addChild(overlayCard);

    auto sliderCard = makeSectionCard(
        "LFSlider",
        "Covers continuous drag, step snapping on release, disabled state, custom thickness, thumb diameter, and programmatic value updates."
    );

    auto continuousValue = makeText("Continuous slider: 35.00", 14.0f, 0xFF3567A3);
    continuousValue->matchParentWidth();
    sliderCard->addChild(continuousValue);

    auto continuousSlider = LFSlider::create(0.0f, 100.0f, 35.0f);
    continuousSlider->matchParentWidth();
    continuousSlider->setTrackThickness(8.0f);
    continuousSlider->setThumbDiameter(24.0f);
    continuousSlider->setStep(0.0f);
    continuousSlider->setTrackColor(0xFFE7EDF5);
    continuousSlider->setProgressColor(0xFF3567A3);
    continuousSlider->setThumbColor(0xFFFFFFFF);
    continuousSlider->setOnValueChanged([continuousValue](float value) {
        setTextFormat(continuousValue, "Continuous slider: %.2f", value);
    });
    sliderCard->addChild(continuousSlider);

    auto steppedValue = makeText("Step slider: 50", 14.0f, 0xFF2F855A);
    steppedValue->matchParentWidth();
    sliderCard->addChild(steppedValue);

    auto steppedSlider = LFSlider::create(0.0f, 100.0f, 50.0f);
    steppedSlider->matchParentWidth();
    steppedSlider->setTrackThickness(10.0f);
    steppedSlider->setThumbDiameter(28.0f);
    steppedSlider->setStep(5.0f);
    steppedSlider->setTrackColor(0xFFE6F4EA);
    steppedSlider->setProgressColor(0xFF2F855A);
    steppedSlider->setThumbColor(0xFFFFFFFF);
    steppedSlider->setOnValueChanged([steppedValue](float value) {
        setTextFormat(steppedValue, "Step slider: %.0f", value);
    });
    sliderCard->addChild(steppedSlider);

    auto compactValue = makeText("Compact slider: -20", 14.0f, 0xFFB45309);
    compactValue->matchParentWidth();
    sliderCard->addChild(compactValue);

    auto compactSlider = LFSlider::create(-50.0f, 50.0f, -20.0f);
    compactSlider->matchParentWidth();
    compactSlider->setTrackThickness(4.0f);
    compactSlider->setThumbDiameter(16.0f);
    compactSlider->setStep(10.0f);
    compactSlider->setTrackColor(0xFFFFF1D6);
    compactSlider->setProgressColor(0xFFB45309);
    compactSlider->setThumbColor(0xFFFFFFFF);
    compactSlider->setOnValueChanged([compactValue](float value) {
        setTextFormat(compactValue, "Compact slider: %.0f", value);
    });
    sliderCard->addChild(compactSlider);

    auto disabledSlider = LFSlider::create(0.0f, 1.0f, 0.65f);
    disabledSlider->matchParentWidth();
    disabledSlider->setTrackThickness(8.0f);
    disabledSlider->setThumbDiameter(22.0f);
    disabledSlider->setProgressColor(0xFF94A3B8);
    disabledSlider->setDisabledOpacity(0.35f);
    disabledSlider->setEnabled(false);
    sliderCard->addChild(makeText("Disabled slider", 14.0f, 0xFF64748B));
    sliderCard->addChild(disabledSlider);

    auto sliderButtons = makeRow();
    auto setLow = makeActionButton("Set Low", [continuousSlider, steppedSlider, compactSlider](LFButton*) {
        continuousSlider->setValue(12.5f);
        steppedSlider->setValue(15.0f);
        compactSlider->setValue(-40.0f);
    });
    auto setHigh = makeActionButton("Set High", [continuousSlider, steppedSlider, compactSlider](LFButton*) {
        continuousSlider->setValue(87.5f);
        steppedSlider->setValue(85.0f);
        compactSlider->setValue(40.0f);
    });
    auto toggleSteppedEnabled = makeActionButton("Toggle Step Enabled", [steppedSlider](LFButton*) {
        steppedSlider->setEnabled(!steppedSlider->isEnabled());
    });
    setLow->setFlexGrow(1.0f);
    setHigh->setFlexGrow(1.0f);
    toggleSteppedEnabled->setFlexGrow(1.0f);
    sliderButtons->addChild(setLow);
    sliderButtons->addChild(setHigh);
    sliderButtons->addChild(toggleSteppedEnabled);
    sliderCard->addChild(sliderButtons);

    page->addChild(sliderCard);

    std::weak_ptr<LFOverlay> weakOverlay = overlay;
    auto showCenter = makeActionButton("Center", [weakOverlay, overlayStatus](LFButton*) {
        if (auto overlay = weakOverlay.lock()) {
            overlay->setContentOffset(0.0f, 0.0f);
            overlay->show(
                makeOverlayCard("Center"),
                LFBoxAlign::Center
            );
            overlayStatus->setText("Overlay state: center");
        }
    });
    auto showTopRight = makeActionButton("Top Right", [weakOverlay, overlayStatus](LFButton*) {
        if (auto overlay = weakOverlay.lock()) {
            overlay->setContentOffset(0.0f, 0.0f);
            overlay->show(
                makeOverlayCard("Top Right"),
                LFBoxAlign::TopRight,
                18.0f,
                18.0f
            );
            overlayStatus->setText("Overlay state: top-right");
        }
    });
    auto showBottomLeft = makeActionButton("Bottom Left", [weakOverlay, overlayStatus](LFButton*) {
        if (auto overlay = weakOverlay.lock()) {
            overlay->setContentOffset(0.0f, 0.0f);
            overlay->show(
                makeOverlayCard("Bottom Left"),
                LFBoxAlign::BottomLeft,
                18.0f,
                18.0f
            );
            overlayStatus->setText("Overlay state: bottom-left");
        }
    });
    showCenter->setFlexGrow(1.0f);
    showTopRight->setFlexGrow(1.0f);
    showBottomLeft->setFlexGrow(1.0f);
    auto overlayRow1 = makeRow();
    overlayRow1->addChild(showCenter);
    overlayRow1->addChild(showTopRight);
    overlayRow1->addChild(showBottomLeft);
    overlayCard->addChild(overlayRow1);

    auto overlayRow2 = makeRow();
    auto showTopLeft = makeActionButton("Top Left", [weakOverlay, overlayStatus](LFButton*) {
        if (auto overlay = weakOverlay.lock()) {
            overlay->setContentOffset(0.0f, 0.0f);
            overlay->show(
                makeOverlayCard("Top Left"),
                LFBoxAlign::TopLeft,
                18.0f,
                18.0f
            );
            overlayStatus->setText("Overlay state: top-left");
        }
    });
    auto showNonModal = makeActionButton("Non Modal", [weakOverlay, overlayStatus](LFButton*) {
        if (auto overlay = weakOverlay.lock()) {
            overlay->setModal(false);
            overlay->show(
                makeOverlayCard("Non Modal"),
                LFBoxAlign::TopCenter,
                0.0f,
                42.0f
            );
            overlayStatus->setText("Overlay state: non-modal top-center");
        }
    });
    auto hideOverlay = makeActionButton("Dismiss", [weakOverlay, overlayStatus](LFButton*) {
        if (auto overlay = weakOverlay.lock()) {
            overlay->dismiss();
            overlayStatus->setText("Overlay state: hidden");
        }
    });
    showTopLeft->setFlexGrow(1.0f);
    showNonModal->setFlexGrow(1.0f);
    hideOverlay->setFlexGrow(1.0f);
    overlayRow2->addChild(showTopLeft);
    overlayRow2->addChild(showNonModal);
    overlayRow2->addChild(hideOverlay);
    overlayCard->addChild(overlayRow2);

    return page;
}

}

std::shared_ptr<ComponentLab> ComponentLab::create() {
    return std::make_shared<ComponentLab>();
}

LFNode::Ptr ComponentLab::start() {
    m_navigator = LFNavigator::create();
    auto tab = LFTab::create();

    auto demoPage = LFPage::create();
    demoPage->setBackgroundColor(0xFFF5F7FB);

    auto demoScrollView = LFScrollView::createVertical();
    demoScrollView->matchParentWidth();
    demoScrollView->matchParentHeight();
    demoScrollView->setBounces(false);
    demoScrollView->addChild(buildFunctionDemoPage());
    demoPage->addChild(demoScrollView);

    tab->addTab("Lab", demoPage, "icon-lab-unselect.png", "icon-lab-selected.png");
    tab->addTab("Developer", ProfilePage::create(), "icon-my-unselect.png", "icon-my-selected.png");

    auto rootPage = LFPage::create();
    rootPage->addChild(tab);
    m_navigator->push(rootPage, false);
    return m_navigator;
}
