#ifndef FILE_PICKER_LEAF_PLUGIN_FILEPICKERPLUGIN_H
#define FILE_PICKER_LEAF_PLUGIN_FILEPICKERPLUGIN_H

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <Leaf_Plugin/LeafPlugin.h>

NS_ASSUME_NONNULL_BEGIN

@interface FilePickerPlugin : NSObject <LeafPlugin, UIDocumentPickerDelegate>

- (instancetype)initWithPresentingViewController:(UIViewController *)presentingViewController NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif // FILE_PICKER_LEAF_PLUGIN_FILEPICKERPLUGIN_H
