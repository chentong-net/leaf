//
//  ViewController.m
//  App
//
//  Created by 陈桐 on 2026/2/19.
//

#import "ViewController.h"
#import <Leaf_iOS/LeafView.h>
#import <Leaf_iOS/LFPluginRegistry.h>
#import <File_Picker/FilePickerPlugin.h>
#import <Path_Provider/PathProviderPlugin.h>
#import <I18n/I18nPlugin.h>
#import <Local_Time/LocalTimePlugin.h>

@interface ViewController ()
@property (nonatomic, strong) LeafView *leafView;
@property (nonatomic, strong) FilePickerPlugin *filePickerPlugin;
@property (nonatomic, strong) PathProviderPlugin *pathProviderPlugin;
@property (nonatomic, strong) I18nPlugin *i18nPlugin;
@property (nonatomic, strong) LocalTimePlugin *localTimePlugin;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.filePickerPlugin = [[FilePickerPlugin alloc] initWithPresentingViewController:self];
    self.pathProviderPlugin = [[PathProviderPlugin alloc] init];
    self.i18nPlugin = [[I18nPlugin alloc] init];
    self.localTimePlugin = [[LocalTimePlugin alloc] init];
    [[LFPluginRegistry sharedInstance] registerPlugin:self.filePickerPlugin];
    [[LFPluginRegistry sharedInstance] registerPlugin:self.pathProviderPlugin];
    [[LFPluginRegistry sharedInstance] registerPlugin:self.i18nPlugin];
    [[LFPluginRegistry sharedInstance] registerPlugin:self.localTimePlugin];

    self.leafView = [[LeafView alloc] initWithFrame:CGRectZero];
    self.leafView.translatesAutoresizingMaskIntoConstraints = NO;
    __weak typeof(self) weakSelf = self;
    self.leafView.onEngineReady = ^{
        NSLog(@"Leaf engine ready in %@", NSStringFromClass([weakSelf class]));
    };

    [self.view addSubview:self.leafView];
    UILayoutGuide *safe = self.view.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [self.leafView.topAnchor constraintEqualToAnchor:safe.topAnchor],
        [self.leafView.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor],
        [self.leafView.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor],
        [self.leafView.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor]
    ]];
}

- (void)dealloc {
    [[LFPluginRegistry sharedInstance] unregisterPlugin:@"FilePickerPlugin"];
    [[LFPluginRegistry sharedInstance] unregisterPlugin:@"PathProviderPlugin"];
    [[LFPluginRegistry sharedInstance] unregisterPlugin:@"I18nPlugin"];
    [[LFPluginRegistry sharedInstance] unregisterPlugin:@"LocalTimePlugin"];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [self.leafView startRendering];
}

- (void)viewWillDisappear:(BOOL)animated {
    [self.leafView stopRendering];
    [super viewWillDisappear:animated];
}

@end
