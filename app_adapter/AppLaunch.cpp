#include "LFAppLaunch.h"
#include "ReaderApp.h"
#include "ProfilePage.h"
#include "LFFileService.h"
#include "view/base/LFPage.h"
#include "view/base/LFText.h"
#include "view/layout/LFLinear.h"
#include "view/wrapped/LFButton.h"

LFNode::Ptr createAppRoot() {
    if (true) {
        // TODO: write code here
        struct DemoState {
            std::string fileId;
            std::string fileName;
            std::string content;
        };

        auto state = std::make_shared<DemoState>();
        auto page = LFPage::create();
        page->setBackgroundColor(0xFFF7F8FA);

        auto root = LFLinear::createVertical();
        root->matchParentWidth();
        root->matchParentHeight();
        root->setPadding(YGEdgeAll, 24.0f);
        root->setSpacing(12.0f);
        root->setGravity(LFAlignment::Start, LFAlignment::Start);

        auto title = std::make_shared<LFText>();
        title->setText("OHOS File Service Demo");
        title->setFontSize(22.0f);
        title->setTextColor(0xFF111827);

        auto desc = std::make_shared<LFText>();
        desc->setText("Pipeline: pick -> read -> save (platform adapter)");
        desc->setFontSize(13.0f);
        desc->setTextColor(0xFF6B7280);

        auto status = std::make_shared<LFText>();
        status->matchParentWidth();
        status->setText("Status: idle");
        status->setFontSize(14.0f);
        status->setTextColor(0xFF111827);

        auto preview = std::make_shared<LFText>();
        preview->matchParentWidth();
        preview->setText("Preview: <none>");
        preview->setFontSize(13.0f);
        preview->setTextColor(0xFF374151);

        auto styleButton = [](const std::shared_ptr<LFButton>& button) {
            button->setWidth(250.0f);
            button->setHeight(44.0f);
            button->setRadius(10.0f);
            button->setTextColor(0xFFFFFFFF);
            button->setBackgroundColor(LFButtonState::Normal, 0xFF2563EB);
            button->setBackgroundColor(LFButtonState::Pressed, 0xFF1D4ED8);
            button->setBackgroundColor(LFButtonState::Disabled, 0xFF9CA3AF);
        };

        auto pickSandboxButton = LFButton::create("Pick File (copyToSandbox=true)", [state, status](LFButton* sender) {
            (void) sender;
            status->setText("Status: opening picker (copyToSandbox=true)...");

            LFFilePickOptions options;
            options.mediaType = LFFileMediaType::Any;
            options.copyToSandbox = true;

            LFFileSystem::pickFile(options, [state, status](const LFFilePickResult& result) {
                if (!result.ok) {
                    if (result.canceled) {
                        status->setText("Status: canceled");
                    } else {
                        status->setText("Status: pick failed - " + result.error);
                    }
                    return;
                }

                if (result.files.empty()) {
                    status->setText("Status: empty result");
                    return;
                }

                const auto& file = result.files.front();
                state->fileId = file.fileId;
                state->fileName = file.name;
                status->setText(
                        "Status: picked id=" + file.fileId +
                        ", hasPath=" + (file.hasLocalPath ? "true" : "false") +
                        ", path=" + (file.path.empty() ? "<empty>" : file.path));
            });
        });
        styleButton(pickSandboxButton);

        auto pickNoSandboxButton = LFButton::create("Pick File (copyToSandbox=false)", [state, status](LFButton* sender) {
            (void) sender;
            status->setText("Status: opening picker (copyToSandbox=false)...");

            LFFilePickOptions options;
            options.mediaType = LFFileMediaType::Any;
            options.copyToSandbox = false;

            LFFileSystem::pickFile(options, [state, status](const LFFilePickResult& result) {
                if (!result.ok) {
                    if (result.canceled) {
                        status->setText("Status: canceled");
                    } else {
                        status->setText("Status: pick failed - " + result.error);
                    }
                    return;
                }

                if (result.files.empty()) {
                    status->setText("Status: empty result");
                    return;
                }

                const auto& file = result.files.front();
                state->fileId = file.fileId;
                state->fileName = file.name;
                status->setText(
                        "Status: picked id=" + file.fileId +
                        ", hasPath=" + (file.hasLocalPath ? "true" : "false") +
                        ", path=" + (file.path.empty() ? "<empty>" : file.path));
            });
        });
        styleButton(pickNoSandboxButton);

        auto readButton = LFButton::create("Read Selected File", [state, status, preview](LFButton* sender) {
            (void) sender;
            if (state->fileId.empty()) {
                status->setText("Status: pick file first");
                return;
            }

            status->setText("Status: reading file...");
            LFFileSystem::readFile(state->fileId, [state, status, preview](const LFFileReadResult& result) {
                if (!result.ok) {
                    if (result.canceled) {
                        status->setText("Status: read canceled");
                    } else {
                        status->setText("Status: read failed - " + result.error);
                    }
                    return;
                }

                state->content = result.content;
                if (state->content.size() > 300) {
                    preview->setText("Preview: " + state->content.substr(0, 300) + " ...");
                } else {
                    preview->setText("Preview: " + state->content);
                }
                status->setText("Status: read success (" + std::to_string(state->content.size()) + " chars)");
            });
        });
        styleButton(readButton);

        auto saveButton = LFButton::create("Save Text To File", [state, status](LFButton* sender) {
            (void) sender;
            LFFileSaveOptions options;
            if (!state->fileName.empty()) {
                options.fileName = "copy_" + state->fileName;
            } else {
                options.fileName = "leaf_output.txt";
            }

            const std::string content = state->content.empty()
                                        ? "Leaf OHOS file service demo."
                                        : state->content;

            status->setText("Status: opening save dialog...");
            LFFileSystem::saveFile(options, content, [status](const LFFileSaveResult& result) {
                if (!result.ok) {
                    if (result.canceled) {
                        status->setText("Status: save canceled");
                    } else {
                        status->setText("Status: save failed - " + result.error);
                    }
                    return;
                }
                status->setText("Status: save success -> " + (result.path.empty() ? "<empty>" : result.path));
            });
        });
        styleButton(saveButton);

        root->addChild(title);
        root->addChild(desc);
        root->addChild(pickSandboxButton);
        root->addChild(pickNoSandboxButton);
        root->addChild(readButton);
        root->addChild(saveButton);
        root->addChild(status);
        root->addChild(preview);
        page->addChild(root);
        return page;
    }
    auto readerApp = ReaderApp::create();
    return readerApp->start();
}
