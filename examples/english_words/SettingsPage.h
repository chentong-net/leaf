//
// EnglishWords settings page.
//

#ifndef ENGLISHWORDS_SETTINGSPAGE_H
#define ENGLISHWORDS_SETTINGSPAGE_H

#include "LFEngine.h"

class LFDropdown;
class LFOverlay;

class SettingsPage : public LFPage {
public:
    static std::shared_ptr<SettingsPage> create();

private:
    void buildUI();
    void buildOverlay();
    void showRestartOverlay();
    void applySelectedLanguage(int index);
    int currentLanguageIndex() const;

    std::shared_ptr<LFDropdown> m_languageDropdown;
    std::shared_ptr<LFOverlay> m_overlay;
};

#endif // ENGLISHWORDS_SETTINGSPAGE_H
