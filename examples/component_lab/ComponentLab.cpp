#include "ComponentLab.h"
#include "LFEngine.h"
#include "LFAudioPlayer.h"
#include "LFCheckbox.h"
#include "LFI18n.h"
#include "LFLocalTime.h"
#include "LFPathProvider.h"
#include "LFRadioGroup.h"
#include "ProfilePage.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
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

LFLinear::Ptr makeTextPreviewBox(const std::string& labelText,
                                 const std::string& bodyText,
                                 int maxLines = 0) {
    auto box = LFLinear::createVertical();
    box->matchParentWidth();
    box->wrapContentHeight();
    box->setPadding(YGEdgeAll, 14.0f);
    box->setSpacing(8.0f);
    box->setBackgroundColor(0xFFF8FAFD);
    box->setBorderRadius(14.0f);
    box->setBorder(1.0f, 0xFFDDE6F0);

    auto label = makeText(labelText, 12.0f, 0xFF64748B);
    label->matchParentWidth();
    box->addChild(label);

    auto body = makeText(bodyText, 14.0f, 0xFF243244);
    body->matchParentWidth();
    body->setLineHeight(1.4f);
    if (maxLines > 0) {
        body->setMaxLines(maxLines);
    }
    box->addChild(body);

    return box;
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

std::string formatAudioTime(double seconds) {
    if (seconds < 0.0) {
        seconds = 0.0;
    }

    const int totalSeconds = static_cast<int>(seconds + 0.5);
    const int minutes = totalSeconds / 60;
    const int remainingSeconds = totalSeconds % 60;

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, remainingSeconds);
    return buffer;
}

std::string formatAudioProgress(double positionSeconds, double durationSeconds) {
    return formatAudioTime(positionSeconds) + " / " + formatAudioTime(durationSeconds);
}

std::string describeLocale(const LFLocale& locale, const char* fallback = "Unavailable") {
    const std::string tag = locale.toTag();
    if (!tag.empty()) {
        return tag;
    }
    return fallback ? fallback : "";
}

std::string formatLocalTimeValue(const LFLocalTimeValue& value) {
    if (value.year <= 0 || value.month <= 0 || value.day <= 0) {
        return "Unavailable";
    }

    char buffer[64];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        value.year,
        value.month,
        value.day,
        value.hour,
        value.minute,
        value.second,
        value.millisecond
    );
    return buffer;
}

std::string formatUtcOffsetMinutes(int offsetMinutes) {
    const int absoluteMinutes = std::abs(offsetMinutes);
    const int hours = absoluteMinutes / 60;
    const int minutes = absoluteMinutes % 60;

    char buffer[24];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "UTC%c%02d:%02d",
        offsetMinutes >= 0 ? '+' : '-',
        hours,
        minutes
    );
    return buffer;
}

void updateLocalTimeDemo(
        const std::shared_ptr<LFText>& currentTimeValue,
        const std::shared_ptr<LFText>& epochMillisValue,
        const std::shared_ptr<LFText>& utcOffsetValue,
        const std::shared_ptr<LFText>& timezoneValue) {
    const LFLocalTimeValue snapshot = LFLocalTime::now();
    currentTimeValue->setText(formatLocalTimeValue(snapshot));
    epochMillisValue->setText(std::to_string(LFLocalTime::nowMillis()));
    utcOffsetValue->setText(formatUtcOffsetMinutes(LFLocalTime::utcOffsetMinutes()));

    const std::string timezone = LFLocalTime::timezone();
    if (!timezone.empty()) {
        timezoneValue->setText(timezone);
    } else if (!snapshot.timezone.empty()) {
        timezoneValue->setText(snapshot.timezone);
    } else {
        timezoneValue->setText("Unavailable");
    }
}

bool writeBinaryFile(const std::filesystem::path& path, const std::shared_ptr<LFData>& data) {
    if (!data || !data->data || data->size == 0) {
        return false;
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output.write(reinterpret_cast<const char*>(data->data), static_cast<std::streamsize>(data->size));
    return output.good();
}

struct AudioDemoState {
    std::shared_ptr<LFAudioPlayer> player;
    bool ready = false;
    bool durationResolved = false;
    bool playing = false;
    bool dragging = false;
    double durationSeconds = 0.0;
    double playbackPositionSeconds = 0.0;
    double dragPreviewPositionSeconds = 0.0;
    double playStartEngineTime = 0.0;
    double playStartPositionSeconds = 0.0;
    double nextDurationProbeTime = 0.0;
    std::string tempAudioPath;
};

double resolveAudioPosition(const std::shared_ptr<AudioDemoState>& state) {
    if (!state) {
        return 0.0;
    }

    double position = state->playbackPositionSeconds;
    if (state->playing) {
        const double elapsed = LFEngine::getInstance().getElapsedTime() - state->playStartEngineTime;
        position = state->playStartPositionSeconds + elapsed;

        if (state->durationSeconds > 0.0) {
            position = std::fmod(position, state->durationSeconds);
            if (position < 0.0) {
                position += state->durationSeconds;
            }
        }
    }

    if (state->durationSeconds > 0.0) {
        position = std::clamp(position, 0.0, state->durationSeconds);
    } else if (position < 0.0) {
        position = 0.0;
    }

    return position;
}

void syncAudioProgress(const std::shared_ptr<AudioDemoState>& state,
                       const LFSlider::Ptr& slider,
                       const std::shared_ptr<LFText>& progressText) {
    if (!state || !slider || !progressText || !state->ready) {
        return;
    }

    const double duration = state->durationSeconds > 0.0 ? state->durationSeconds : 1.0;
    const double position = state->dragging ? state->dragPreviewPositionSeconds : resolveAudioPosition(state);

    slider->setRange(0.0f, static_cast<float>(duration));
    if (!state->dragging) {
        slider->setValue(static_cast<float>(position), false);
        state->playbackPositionSeconds = position;
    }

    progressText->setText(formatAudioProgress(position, duration));
}

void commitAudioSeek(const std::shared_ptr<AudioDemoState>& state,
                     const LFSlider::Ptr& slider,
                     const std::shared_ptr<LFText>& progressText) {
    if (!state || !slider || !state->player || !state->ready) {
        return;
    }

    double nextPosition = static_cast<double>(slider->getValue());
    if (state->durationSeconds > 0.0) {
        nextPosition = std::clamp(nextPosition, 0.0, state->durationSeconds);
    } else if (nextPosition < 0.0) {
        nextPosition = 0.0;
    }

    state->playbackPositionSeconds = nextPosition;
    state->playStartPositionSeconds = nextPosition;
    state->playStartEngineTime = LFEngine::getInstance().getElapsedTime();

    state->player->seek(nextPosition);
    if (progressText) {
        const double duration = state->durationSeconds > 0.0 ? state->durationSeconds : 1.0;
        progressText->setText(formatAudioProgress(nextPosition, duration));
    }
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
        "Use this page to validate LFDropdown, LFToggle, LFCheckbox, LFRadioGroup, LFOverlay, LFSlider, and LFAudioPlayer across layout, state, disabled, and interaction cases.",
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

    auto choiceCard = makeSectionCard(
        "LFCheckbox / LFRadioGroup",
        "Covers whole-row click, checked callback, programmatic state changes, horizontal and vertical radio layouts, clearing selection, and disabled state."
    );

    auto checkboxStatus = makeText("Checked options: Remember learned words", 14.0f, 0xFF3567A3);
    checkboxStatus->matchParentWidth();
    choiceCard->addChild(checkboxStatus);

    auto rememberCheckbox = LFCheckbox::create("Remember learned words", true);
    rememberCheckbox->matchParentWidth();
    rememberCheckbox->setBoxSize(22.0f);
    rememberCheckbox->setTextColor(0xFF334155, 0xFF0F5B99);
    rememberCheckbox->setIndicatorColor(0xFFFFFFFF, 0xFF2563EB);
    rememberCheckbox->setBorderColor(0xFFCBD5E1, 0xFF2563EB);
    choiceCard->addChild(rememberCheckbox);

    auto pronunciationCheckbox = LFCheckbox::create("Auto play pronunciation", false);
    pronunciationCheckbox->matchParentWidth();
    pronunciationCheckbox->setBoxSize(22.0f);
    pronunciationCheckbox->setIndicatorColor(0xFFFFFFFF, 0xFF0EA5E9);
    pronunciationCheckbox->setBorderColor(0xFFCBD5E1, 0xFF0284C7);
    pronunciationCheckbox->setTextColor(0xFF334155, 0xFF0369A1);
    choiceCard->addChild(pronunciationCheckbox);

    auto disabledCheckbox = LFCheckbox::create("Disabled checkbox", true);
    disabledCheckbox->matchParentWidth();
    disabledCheckbox->setEnabled(false);
    disabledCheckbox->setDisabledOpacity(0.32f);
    choiceCard->addChild(disabledCheckbox);

    auto updateCheckboxStatus = [checkboxStatus, rememberCheckbox, pronunciationCheckbox]() {
        std::string summary = "Checked options:";
        bool hasAny = false;
        if (rememberCheckbox->isChecked()) {
            summary += " Remember learned words";
            hasAny = true;
        }
        if (pronunciationCheckbox->isChecked()) {
            summary += hasAny ? " | Auto play pronunciation" : " Auto play pronunciation";
            hasAny = true;
        }
        if (!hasAny) {
            summary += " none";
        }
        checkboxStatus->setText(summary);
    };

    rememberCheckbox->setOnCheckedChanged([updateCheckboxStatus](bool) {
        updateCheckboxStatus();
    });
    pronunciationCheckbox->setOnCheckedChanged([updateCheckboxStatus](bool) {
        updateCheckboxStatus();
    });

    auto checkboxControls = makeRow();
    auto enableAllCheckboxes = makeActionButton("Check All", [rememberCheckbox, pronunciationCheckbox](LFButton*) {
        rememberCheckbox->setChecked(true);
        pronunciationCheckbox->setChecked(true);
    });
    auto clearAllCheckboxes = makeActionButton("Clear All", [rememberCheckbox, pronunciationCheckbox](LFButton*) {
        rememberCheckbox->setChecked(false);
        pronunciationCheckbox->setChecked(false);
    });
    enableAllCheckboxes->setFlexGrow(1.0f);
    clearAllCheckboxes->setFlexGrow(1.0f);
    checkboxControls->addChild(enableAllCheckboxes);
    checkboxControls->addChild(clearAllCheckboxes);
    choiceCard->addChild(checkboxControls);

    auto radioStatus = makeText("Vertical radio: Normal", 14.0f, 0xFF7C3AED);
    radioStatus->matchParentWidth();
    choiceCard->addChild(radioStatus);

    auto difficultyGroup = LFRadioGroup::create({"Easy", "Normal", "Hard"});
    difficultyGroup->matchParentWidth();
    difficultyGroup->setSelectedIndex(1, false);
    difficultyGroup->setOptionSpacing(8.0f);
    difficultyGroup->setItemSpacing(10.0f);
    difficultyGroup->setIndicatorColor(0xFFFFFFFF, 0xFFF3E8FF);
    difficultyGroup->setBorderColor(0xFFD8B4FE, 0xFF7C3AED);
    difficultyGroup->setTextColor(0xFF334155, 0xFF6D28D9);
    difficultyGroup->setItemBackgroundColor(0x00000000, 0x122563EB);
    difficultyGroup->setOnSelectionChanged([radioStatus](int, const std::string& value) {
        radioStatus->setText("Vertical radio: " + value);
    });
    choiceCard->addChild(difficultyGroup);

    auto horizontalStatus = makeText("Horizontal radio: English -> Chinese", 14.0f, 0xFF0F766E);
    horizontalStatus->matchParentWidth();
    choiceCard->addChild(horizontalStatus);

    auto modeGroup = LFRadioGroup::create({"English -> Chinese", "Chinese -> English", "Spelling"});
    modeGroup->setOptionDirection(LFOrientation::Horizontal);
    modeGroup->setOptionSpacing(12.0f);
    modeGroup->setItemSpacing(8.0f);
    modeGroup->setSelectedIndex(0, false);
    modeGroup->setIndicatorSize(18.0f);
    modeGroup->setInnerDotSize(8.0f);
    modeGroup->setIndicatorColor(0xFFFFFFFF, 0xFFE6FFFB);
    modeGroup->setBorderColor(0xFF99F6E4, 0xFF0F766E);
    modeGroup->setTextColor(0xFF334155, 0xFF0F766E);
    modeGroup->setItemBackgroundColor(0x00000000, 0x140F766E);
    modeGroup->setOnSelectionChanged([horizontalStatus](int, const std::string& value) {
        horizontalStatus->setText("Horizontal radio: " + value);
    });
    choiceCard->addChild(modeGroup);

    auto disabledGroup = LFRadioGroup::create({"Disabled A", "Disabled B"});
    disabledGroup->matchParentWidth();
    disabledGroup->setSelectedIndex(0, false);
    disabledGroup->setEnabled(false);
    disabledGroup->setDisabledOpacity(0.32f);
    choiceCard->addChild(disabledGroup);

    auto radioControls = makeRow();
    auto selectHard = makeActionButton("Select Hard", [difficultyGroup](LFButton*) {
        difficultyGroup->setSelectedIndex(2);
    });
    auto clearRadio = makeActionButton("Clear Radio", [difficultyGroup, radioStatus](LFButton*) {
        difficultyGroup->clearSelection(false);
        radioStatus->setText("Vertical radio: none");
    });
    auto selectSpelling = makeActionButton("Select Spelling", [modeGroup](LFButton*) {
        modeGroup->setSelectedIndex(2);
    });
    selectHard->setFlexGrow(1.0f);
    clearRadio->setFlexGrow(1.0f);
    selectSpelling->setFlexGrow(1.0f);
    radioControls->addChild(selectHard);
    radioControls->addChild(clearRadio);
    radioControls->addChild(selectSpelling);
    choiceCard->addChild(radioControls);

    page->addChild(choiceCard);

    auto textCard = makeSectionCard(
        "LFText",
        "Covers default wrapping and maxLines clip behavior while keeping the current engine-side line-break pipeline."
    );

    const std::string textSample =
        "This LFText sample is intentionally long so it wraps into multiple lines on narrow layouts and can be clipped cleanly when maxLines is enabled.";

    textCard->addChild(makeTextPreviewBox("No maxLines", textSample));
    textCard->addChild(makeTextPreviewBox("maxLines = 2", textSample, 2));
    textCard->addChild(makeTextPreviewBox("maxLines = 1", textSample, 1));

    auto runtimeTextStatus = makeText("Runtime preview: unlimited", 14.0f, 0xFF3567A3);
    runtimeTextStatus->matchParentWidth();
    textCard->addChild(runtimeTextStatus);

    auto runtimeTextBox = LFLinear::createVertical();
    runtimeTextBox->matchParentWidth();
    runtimeTextBox->wrapContentHeight();
    runtimeTextBox->setPadding(YGEdgeAll, 14.0f);
    runtimeTextBox->setBackgroundColor(0xFFF8FAFD);
    runtimeTextBox->setBorderRadius(14.0f);
    runtimeTextBox->setBorder(1.0f, 0xFFDDE6F0);

    auto runtimeText = makeText(textSample, 14.0f, 0xFF243244);
    runtimeText->matchParentWidth();
    runtimeText->setLineHeight(1.4f);
    runtimeTextBox->addChild(runtimeText);
    textCard->addChild(runtimeTextBox);

    auto textControlRow1 = makeRow();
    auto textNoLimitButton = makeActionButton("No Limit", [runtimeText, runtimeTextStatus](LFButton*) {
        runtimeText->setMaxLines(0);
        runtimeTextStatus->setText("Runtime preview: unlimited");
    });
    auto textOneLineButton = makeActionButton("1 Line", [runtimeText, runtimeTextStatus](LFButton*) {
        runtimeText->setMaxLines(1);
        runtimeTextStatus->setText("Runtime preview: maxLines = 1");
    });
    textNoLimitButton->setFlexGrow(1.0f);
    textOneLineButton->setFlexGrow(1.0f);
    textControlRow1->addChild(textNoLimitButton);
    textControlRow1->addChild(textOneLineButton);
    textCard->addChild(textControlRow1);

    auto textControlRow2 = makeRow();
    auto textTwoLinesButton = makeActionButton("2 Lines", [runtimeText, runtimeTextStatus](LFButton*) {
        runtimeText->setMaxLines(2);
        runtimeTextStatus->setText("Runtime preview: maxLines = 2");
    });
    auto textThreeLinesButton = makeActionButton("3 Lines", [runtimeText, runtimeTextStatus](LFButton*) {
        runtimeText->setMaxLines(3);
        runtimeTextStatus->setText("Runtime preview: maxLines = 3");
    });
    textTwoLinesButton->setFlexGrow(1.0f);
    textThreeLinesButton->setFlexGrow(1.0f);
    textControlRow2->addChild(textTwoLinesButton);
    textControlRow2->addChild(textThreeLinesButton);
    textCard->addChild(textControlRow2);

    page->addChild(textCard);

    auto i18nCard = makeSectionCard(
        "LFI18n",
        "Loads i18n.json via LFResourceProvider, applies system-language defaults, supports manual setLanguage, and demonstrates default-language fallback."
    );

    auto i18nStatus = makeText("", 13.0f, 0xFF475569);
    i18nStatus->matchParentWidth();
    i18nCard->addChild(i18nStatus);

    auto i18nTitle = makeText("", 18.0f, 0xFF142033);
    i18nTitle->matchParentWidth();
    i18nCard->addChild(i18nTitle);

    auto i18nDescription = makeText("", 13.0f, 0xFF637083);
    i18nDescription->matchParentWidth();
    i18nCard->addChild(i18nDescription);

    auto systemLanguageRow = makeRow();
    auto systemLanguageLabel = makeText("", 14.0f, 0xFF334155);
    systemLanguageLabel->setFlexGrow(1.0f);
    auto systemLanguageValue = makeText("...", 14.0f, 0xFF0F5B99);
    systemLanguageRow->addChild(systemLanguageLabel);
    systemLanguageRow->addChild(systemLanguageValue);
    i18nCard->addChild(systemLanguageRow);

    auto currentLanguageRow = makeRow();
    auto currentLanguageLabel = makeText("", 14.0f, 0xFF334155);
    currentLanguageLabel->setFlexGrow(1.0f);
    auto currentLanguageValue = makeText("...", 14.0f, 0xFF0F5B99);
    currentLanguageRow->addChild(currentLanguageLabel);
    currentLanguageRow->addChild(currentLanguageValue);
    i18nCard->addChild(currentLanguageRow);

    auto manualSwitchTip = makeText("", 12.0f, 0xFF64748B);
    manualSwitchTip->matchParentWidth();
    i18nCard->addChild(manualSwitchTip);

    auto sampleTextLabel = makeText("", 13.0f, 0xFF64748B);
    sampleTextLabel->matchParentWidth();

    auto samplePrimary = makeText("", 14.0f, 0xFF243244);
    samplePrimary->matchParentWidth();
    samplePrimary->setLineHeight(1.4f);

    auto sampleSecondary = makeText("", 14.0f, 0xFF243244);
    sampleSecondary->matchParentWidth();
    sampleSecondary->setLineHeight(1.4f);

    auto fallbackLabel = makeText("", 13.0f, 0xFF8B5E00);
    fallbackLabel->matchParentWidth();

    auto fallbackValue = makeText("", 14.0f, 0xFF7C2D12);
    fallbackValue->matchParentWidth();
    fallbackValue->setLineHeight(1.4f);

    auto refreshI18nPreview = [
        i18nStatus,
        i18nTitle,
        i18nDescription,
        systemLanguageLabel,
        systemLanguageValue,
        currentLanguageLabel,
        currentLanguageValue,
        manualSwitchTip,
        sampleTextLabel,
        samplePrimary,
        sampleSecondary,
        fallbackLabel,
        fallbackValue
    ]() {
        const bool ready = LFI18n::isReady();

        i18nStatus->setText(ready ? "i18n: ready" : "i18n: load failed, using key fallback");
        i18nTitle->setText(LFI18n::get("i18n.demo.title"));
        i18nDescription->setText(LFI18n::get("i18n.demo.description"));
        systemLanguageLabel->setText(LFI18n::get("i18n.demo.system_language_label"));
        systemLanguageValue->setText(describeLocale(LFI18n::getSystemLanguage(), "Unavailable"));
        currentLanguageLabel->setText(LFI18n::get("i18n.demo.current_language_label"));
        currentLanguageValue->setText(describeLocale(LFI18n::getCurrentLanguage(), "Unavailable"));
        manualSwitchTip->setText(LFI18n::get("i18n.demo.manual_switch_tip"));
        sampleTextLabel->setText(LFI18n::get("i18n.demo.sample_label"));
        samplePrimary->setText(LFI18n::get("i18n.demo.sample_primary"));
        sampleSecondary->setText(LFI18n::get("i18n.demo.sample_secondary"));
        fallbackLabel->setText(LFI18n::get("i18n.demo.default_fallback_label"));
        fallbackValue->setText(LFI18n::get("i18n.demo.default_fallback_value"));
    };

    auto i18nButtonRow = makeRow();
    auto setZhButton = makeActionButton("zh-CN", [refreshI18nPreview](LFButton*) {
        LFI18n::setLanguage(LFLocales::ZhCN);
        refreshI18nPreview();
    });
    auto setEnButton = makeActionButton("en-US", [refreshI18nPreview](LFButton*) {
        LFI18n::setLanguage(LFLocales::EnUS);
        refreshI18nPreview();
    });
    auto setRuButton = makeActionButton("ru-RU", [refreshI18nPreview](LFButton*) {
        LFI18n::setLanguage(LFLocales::RuRU);
        refreshI18nPreview();
    });
    setZhButton->setFlexGrow(1.0f);
    setEnButton->setFlexGrow(1.0f);
    setRuButton->setFlexGrow(1.0f);
    i18nButtonRow->addChild(setZhButton);
    i18nButtonRow->addChild(setEnButton);
    i18nButtonRow->addChild(setRuButton);
    i18nCard->addChild(i18nButtonRow);

    i18nCard->addChild(sampleTextLabel);

    i18nCard->addChild(samplePrimary);

    i18nCard->addChild(sampleSecondary);

    i18nCard->addChild(fallbackLabel);

    i18nCard->addChild(fallbackValue);

    refreshI18nPreview();

    page->addChild(i18nCard);

    auto localTimeCard = makeSectionCard(
        "LFLocalTime",
        "Reads the current local clock, epoch milliseconds, UTC offset, and timezone on Desktop and Android."
    );

    auto localTimeStatus = makeText("Local time: auto updates every second.", 13.0f, 0xFF475569);
    localTimeStatus->matchParentWidth();
    localTimeCard->addChild(localTimeStatus);

    auto currentTimeRow = makeRow();
    auto currentTimeLabel = makeText("Current time", 14.0f, 0xFF334155);
    currentTimeLabel->setFlexGrow(1.0f);
    auto currentTimeValue = makeText("...", 14.0f, 0xFF0F5B99);
    currentTimeRow->addChild(currentTimeLabel);
    currentTimeRow->addChild(currentTimeValue);
    localTimeCard->addChild(currentTimeRow);

    auto epochMillisRow = makeRow();
    auto epochMillisLabel = makeText("Epoch millis", 14.0f, 0xFF334155);
    epochMillisLabel->setFlexGrow(1.0f);
    auto epochMillisValue = makeText("...", 14.0f, 0xFF0F5B99);
    epochMillisRow->addChild(epochMillisLabel);
    epochMillisRow->addChild(epochMillisValue);
    localTimeCard->addChild(epochMillisRow);

    auto utcOffsetRow = makeRow();
    auto utcOffsetLabel = makeText("UTC offset", 14.0f, 0xFF334155);
    utcOffsetLabel->setFlexGrow(1.0f);
    auto utcOffsetValue = makeText("...", 14.0f, 0xFF0F5B99);
    utcOffsetRow->addChild(utcOffsetLabel);
    utcOffsetRow->addChild(utcOffsetValue);
    localTimeCard->addChild(utcOffsetRow);

    auto timezoneRow = makeRow();
    auto timezoneLabel = makeText("Timezone", 14.0f, 0xFF334155);
    timezoneLabel->setFlexGrow(1.0f);
    auto timezoneValue = makeText("...", 14.0f, 0xFF0F5B99);
    timezoneRow->addChild(timezoneLabel);
    timezoneRow->addChild(timezoneValue);
    localTimeCard->addChild(timezoneRow);

    auto localTimeRefreshButton = makeActionButton("Refresh Snapshot", [currentTimeValue, epochMillisValue, utcOffsetValue, timezoneValue](LFButton*) {
        updateLocalTimeDemo(currentTimeValue, epochMillisValue, utcOffsetValue, timezoneValue);
    });
    localTimeCard->addChild(localTimeRefreshButton);

    updateLocalTimeDemo(currentTimeValue, epochMillisValue, utcOffsetValue, timezoneValue);

    auto localTimeLastSecond = std::make_shared<int64_t>(-1);
    std::weak_ptr<LFText> weakCurrentTimeValue = currentTimeValue;
    std::weak_ptr<LFText> weakEpochMillisValue = epochMillisValue;
    std::weak_ptr<LFText> weakUtcOffsetValue = utcOffsetValue;
    std::weak_ptr<LFText> weakTimezoneValue = timezoneValue;

    LFEngine::getInstance().addFrameTask([
        weakCurrentTimeValue,
        weakEpochMillisValue,
        weakUtcOffsetValue,
        weakTimezoneValue,
        localTimeLastSecond
    ]() mutable {
        auto currentTimeText = weakCurrentTimeValue.lock();
        auto epochMillisText = weakEpochMillisValue.lock();
        auto utcOffsetText = weakUtcOffsetValue.lock();
        auto timezoneText = weakTimezoneValue.lock();
        if (!currentTimeText || !epochMillisText || !utcOffsetText || !timezoneText) {
            return false;
        }

        const int64_t currentSecond = LFLocalTime::nowMillis() / 1000;
        if (currentSecond == *localTimeLastSecond) {
            return true;
        }

        *localTimeLastSecond = currentSecond;
        updateLocalTimeDemo(currentTimeText, epochMillisText, utcOffsetText, timezoneText);
        return true;
    });

    page->addChild(localTimeCard);

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

    auto audioCard = makeSectionCard(
        "LFAudioPlayer",
        "Loads test-music.mp3 through LFResourceProvider, writes it to a temporary file, and tests looping playback with drag-end seek."
    );

    auto audioStatus = makeText("Audio: loading...", 13.0f, 0xFF475569);
    audioStatus->matchParentWidth();
    audioCard->addChild(audioStatus);

    auto audioProgress = makeText("00:00 / 00:00", 14.0f, 0xFF1D4ED8);
    audioProgress->matchParentWidth();
    audioCard->addChild(audioProgress);

    auto audioSlider = LFSlider::create(0.0f, 1.0f, 0.0f);
    audioSlider->matchParentWidth();
    audioSlider->setTrackThickness(8.0f);
    audioSlider->setThumbDiameter(22.0f);
    audioSlider->setTrackColor(0xFFE2E8F0);
    audioSlider->setProgressColor(0xFF2563EB);
    audioSlider->setThumbColor(0xFFFFFFFF);
    audioSlider->setEnabled(false);
    audioCard->addChild(audioSlider);

    auto audioButton = makeActionButton("播放/暂停", nullptr);
    audioButton->setFontSize(13.0f);
    audioButton->setEnabled(false);
    audioCard->addChild(audioButton);

    auto audioState = std::make_shared<AudioDemoState>();
    audioState->player = LFAudioPlayer::create();

    std::weak_ptr<AudioDemoState> weakAudioState = audioState;
    std::weak_ptr<LFSlider> weakAudioSlider = audioSlider;
    std::weak_ptr<LFText> weakAudioStatus = audioStatus;
    std::weak_ptr<LFText> weakAudioProgress = audioProgress;
    std::weak_ptr<LFButton> weakAudioButton = audioButton;

    audioState->player->setOnError([weakAudioState, weakAudioStatus, weakAudioButton, weakAudioSlider](const LFAudioPlayerEvent&) {
        auto state = weakAudioState.lock();
        auto status = weakAudioStatus.lock();
        auto button = weakAudioButton.lock();
        auto slider = weakAudioSlider.lock();

        if (state) {
            state->ready = false;
            state->playing = false;
            state->dragging = false;
        }

        if (status) {
            status->setText("Audio: load failed");
        }
        if (button) {
            button->setEnabled(false);
        }
        if (slider) {
            slider->setEnabled(false);
        }
    });

    audioSlider->setOnDragBegin([audioState](float value) {
        audioState->dragging = true;
        audioState->dragPreviewPositionSeconds = static_cast<double>(value);
    });

    audioSlider->setOnValueChanged([audioState, weakAudioProgress](float value) {
        if (audioState->dragging) {
            audioState->dragPreviewPositionSeconds = static_cast<double>(value);
            if (auto progress = weakAudioProgress.lock()) {
                const double duration = audioState->durationSeconds > 0.0 ? audioState->durationSeconds : 1.0;
                progress->setText(formatAudioProgress(audioState->dragPreviewPositionSeconds, duration));
            }
        }
    });

    audioSlider->setOnDragEnd([audioState, weakAudioSlider, weakAudioProgress](float value) {
        auto slider = weakAudioSlider.lock();
        audioState->dragging = false;
        audioState->dragPreviewPositionSeconds = static_cast<double>(value);
        if (slider) {
            commitAudioSeek(audioState, slider, weakAudioProgress.lock());
        }
    });

    audioButton->setOnClick([audioState, weakAudioStatus, weakAudioSlider, weakAudioProgress](LFButton*) {
        auto status = weakAudioStatus.lock();
        auto slider = weakAudioSlider.lock();
        auto progress = weakAudioProgress.lock();
        if (!audioState->player || !audioState->ready) {
            return;
        }

        if (audioState->playing) {
            audioState->playbackPositionSeconds = resolveAudioPosition(audioState);
            audioState->playStartPositionSeconds = audioState->playbackPositionSeconds;
            audioState->player->pause();
            audioState->playing = false;
            if (status) {
                status->setText("Audio: paused");
            }
        } else {
            audioState->playStartPositionSeconds = audioState->playbackPositionSeconds;
            audioState->playStartEngineTime = LFEngine::getInstance().getElapsedTime();
            audioState->player->play();
            audioState->playing = true;
            if (status) {
                status->setText("Audio: playing");
            }
        }

        if (slider && progress) {
            syncAudioProgress(audioState, slider, progress);
        }
    });

    page->addChild(audioCard);

    LFPathProvider::getTemporaryPath([weakAudioState, weakAudioStatus, weakAudioSlider, weakAudioProgress, weakAudioButton](const LFPathProviderResult& result) {
        auto state = weakAudioState.lock();
        auto status = weakAudioStatus.lock();
        auto slider = weakAudioSlider.lock();
        auto progress = weakAudioProgress.lock();
        auto button = weakAudioButton.lock();
        if (!state || !status || !slider || !progress || !button || !state->player) {
            return;
        }

        std::filesystem::path tempDir;
        if (result.ok && !result.path.empty()) {
            tempDir = std::filesystem::u8path(result.path);
        } else {
            std::error_code ec;
            tempDir = std::filesystem::temp_directory_path(ec);
            if (ec) {
                status->setText("Audio: load failed");
                return;
            }
        }

        const std::filesystem::path tempAudioPath = tempDir / "leaf_component_lab_test_music.mp3";
        state->tempAudioPath = tempAudioPath.u8string();

        LFResourceProvider::getInstance().fetchAsset("test-music.mp3", [weakAudioState, weakAudioStatus, weakAudioSlider, weakAudioProgress, weakAudioButton, tempAudioPath](std::shared_ptr<LFData> data) {
            auto state = weakAudioState.lock();
            auto status = weakAudioStatus.lock();
            auto slider = weakAudioSlider.lock();
            auto progress = weakAudioProgress.lock();
            auto button = weakAudioButton.lock();
            if (!state || !status || !slider || !progress || !button || !state->player) {
                return;
            }

            if (!writeBinaryFile(tempAudioPath, data)) {
                status->setText("Audio: load failed");
                button->setEnabled(false);
                slider->setEnabled(false);
                return;
            }

            state->player->setLooping(true);
            state->player->setVolume(1.0f);
            state->player->setSource(tempAudioPath.u8string());

#if defined(__DESKTOP__)
            state->durationSeconds = state->player->getDuration();
            state->durationResolved = state->durationSeconds > 0.0;
            if (state->durationSeconds <= 0.0) {
                state->durationSeconds = 1.0;
            }
#else
            state->durationSeconds = 1.0;
            state->durationResolved = false;
#endif
            state->playbackPositionSeconds = 0.0;
            state->dragPreviewPositionSeconds = 0.0;
            state->playStartPositionSeconds = 0.0;
            state->playStartEngineTime = LFEngine::getInstance().getElapsedTime();
            state->nextDurationProbeTime = state->playStartEngineTime + 0.2;
            state->ready = true;
            state->playing = false;
            state->dragging = false;

            slider->setEnabled(true);
            button->setEnabled(true);
            status->setText("Audio: ready");
            syncAudioProgress(state, slider, progress);
        });
    });

    LFEngine::getInstance().addFrameTask([weakAudioState, weakAudioSlider, weakAudioProgress]() mutable {
        auto state = weakAudioState.lock();
        auto slider = weakAudioSlider.lock();
        auto progress = weakAudioProgress.lock();
        if (!state || !slider || !progress) {
            return false;
        }

        if (state->ready) {
            const double now = LFEngine::getInstance().getElapsedTime();
            if (!state->durationResolved && state->player && now >= state->nextDurationProbeTime) {
                state->nextDurationProbeTime = now + 0.5;
                const double duration = state->player->getDuration();
                if (duration > 0.0) {
                    state->durationResolved = true;
                    state->durationSeconds = duration;
                }
            }
            syncAudioProgress(state, slider, progress);
        }

        return true;
    });

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
