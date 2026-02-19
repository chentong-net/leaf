#import "LFPluginRegistry.h"
#if __has_include(<Leaf_Plugin/LeafPlugin.h>)
#import <Leaf_Plugin/LeafPlugin.h>
#else
#import "LeafPlugin.h"
#endif

@interface LFPluginRegistry ()
@property (nonatomic, strong) NSMutableDictionary<NSString *, id<LeafPlugin>> *pluginMap;
@end

@implementation LFPluginRegistry

+ (instancetype)sharedInstance {
    static LFPluginRegistry *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[LFPluginRegistry alloc] initPrivate];
    });
    return instance;
}

- (instancetype)init {
    [NSException raise:NSInternalInconsistencyException format:@"Use +[LFPluginRegistry sharedInstance]."];
    return nil;
}

- (instancetype)initPrivate {
    self = [super init];
    if (self) {
        _pluginMap = [NSMutableDictionary dictionary];
    }
    return self;
}

- (void)registerPlugin:(id<LeafPlugin>)plugin {
    if (plugin == nil) {
        return;
    }
    NSString *pluginName = LFRequirePluginName([plugin pluginName]);
    @synchronized (self) {
        self.pluginMap[pluginName] = plugin;
    }
}

- (void)unregisterPlugin:(NSString *)pluginName {
    if (pluginName == nil || pluginName.length == 0) {
        return;
    }
    @synchronized (self) {
        [self.pluginMap removeObjectForKey:pluginName];
    }
}

- (void)clear {
    @synchronized (self) {
        [self.pluginMap removeAllObjects];
    }
}

- (id<LeafPlugin>)findByMethod:(NSString *)method {
    if (method == nil || method.length == 0) {
        return nil;
    }

    NSArray<id<LeafPlugin>> *plugins = nil;
    @synchronized (self) {
        plugins = [self.pluginMap.allValues copy];
    }

    for (id<LeafPlugin> plugin in plugins) {
        @try {
            if ([plugin canHandle:method]) {
                return plugin;
            }
        } @catch (__unused NSException *exception) {
            // Plugin match exceptions should not block other plugins.
        }
    }
    return nil;
}

- (NSArray<NSString *> *)pluginNames {
    @synchronized (self) {
        return [self.pluginMap.allKeys copy];
    }
}

@end
