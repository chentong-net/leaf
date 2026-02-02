//
// Created by Chen Tong on 2026/2/2.
//

#ifndef PROFILEPAGE_H
#define PROFILEPAGE_H

#include <memory>
#include "LFPage.h"

class ProfilePage : public LFPage {
public:
    static std::shared_ptr<ProfilePage> create();

private:
    void initUI();
};

#endif // PROFILEPAGE_H
