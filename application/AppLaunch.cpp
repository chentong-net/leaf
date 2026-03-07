#include "LFAppLaunch.h"
#include "ReaderApp.h"
#include "ProfilePage.h"

LFNode::Ptr createAppRoot() {
    auto readerApp = ReaderApp::create();
    return readerApp->start();
}
