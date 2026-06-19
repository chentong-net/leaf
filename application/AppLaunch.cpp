#include "LFAppLaunch.h"
#include "ReaderApp.h"
#include "ComponentLab.h"
#include "ProfilePage.h"

LFNode::Ptr createAppRoot() {
    auto labApp = ComponentLab::create();
    return labApp->start();
}
