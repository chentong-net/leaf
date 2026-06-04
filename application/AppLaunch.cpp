#include "LFAppLaunch.h"
#include "EnglishWordsApp.h"

LFNode::Ptr createAppRoot() {
    return EnglishWordsApp::start();
}
