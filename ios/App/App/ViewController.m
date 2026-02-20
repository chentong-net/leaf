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

@interface ViewController ()
@property (nonatomic, strong) LeafView *leafView;
@property (nonatomic, strong) FilePickerPlugin *filePickerPlugin;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.filePickerPlugin = [[FilePickerPlugin alloc] initWithPresentingViewController:self];
    [[LFPluginRegistry sharedInstance] registerPlugin:self.filePickerPlugin];

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
