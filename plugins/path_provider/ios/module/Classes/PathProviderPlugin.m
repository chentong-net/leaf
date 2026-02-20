#import "PathProviderPlugin.h"
#import <TargetConditionals.h>

static const int32_t REQUEST_ERROR_INTERNAL = -500;
static const int32_t REQUEST_ERROR_METHOD_NOT_IMPLEMENTED = -404;

@implementation PathProviderPlugin

- (NSString *)pluginName {
    return @"PathProviderPlugin";
}

- (BOOL)canHandle:(NSString *)method {
    return [method isEqualToString:@"path_provider.get_temporary_path"]
        || [method isEqualToString:@"path_provider.get_application_support_path"]
        || [method isEqualToString:@"path_provider.get_application_documents_path"]
        || [method isEqualToString:@"path_provider.get_downloads_path"]
        || [method isEqualToString:@"path_provider.get_external_storage_path"];
}

- (void)onMethodCall:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSString *method = call.method ?: @"";
    NSString *path = nil;

    if ([method isEqualToString:@"path_provider.get_temporary_path"]) {
        path = [self temporaryPath];
    } else if ([method isEqualToString:@"path_provider.get_application_support_path"]) {
        path = [self applicationSupportPath];
    } else if ([method isEqualToString:@"path_provider.get_application_documents_path"]) {
        path = [self applicationDocumentsPath];
    } else if ([method isEqualToString:@"path_provider.get_downloads_path"]) {
        path = [self downloadsPath];
    } else if ([method isEqualToString:@"path_provider.get_external_storage_path"]) {
        path = @"";
    } else {
        [result error:call.requestId
                 code:REQUEST_ERROR_METHOD_NOT_IMPLEMENTED
                error:@"method_not_implemented"
             canceled:NO];
        return;
    }

    NSDictionary *payload = @{@"path": path ?: @""};
    NSString *json = [self jsonStringFromDictionary:payload];
    if (!json) {
        [result error:call.requestId
                 code:REQUEST_ERROR_INTERNAL
                error:@"serialize_result_failed"
             canceled:NO];
        return;
    }
    [result success:call.requestId data:json];
}

- (NSString *)temporaryPath {
    NSString *path = NSTemporaryDirectory();
    return path ?: @"";
}

- (NSString *)applicationSupportPath {
    NSString *path = [self firstPathForDirectory:NSApplicationSupportDirectory];
    if (path.length == 0) {
        return @"";
    }

    NSError *error = nil;
    [[NSFileManager defaultManager] createDirectoryAtPath:path
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:&error];
    return path;
}

- (NSString *)applicationDocumentsPath {
    return [self firstPathForDirectory:NSDocumentDirectory];
}

- (NSString *)downloadsPath {
#if TARGET_OS_MACCATALYST
    return [self firstPathForDirectory:NSDownloadsDirectory];
#else
    return @"";
#endif
}

- (NSString *)firstPathForDirectory:(NSSearchPathDirectory)directory {
    NSArray<NSString *> *paths = NSSearchPathForDirectoriesInDomains(directory, NSUserDomainMask, YES);
    NSString *path = paths.firstObject;
    return path ?: @"";
}

- (nullable NSString *)jsonStringFromDictionary:(NSDictionary *)dictionary {
    NSError *error = nil;
    NSData *data = [NSJSONSerialization dataWithJSONObject:dictionary options:0 error:&error];
    if (!data || error) {
        return nil;
    }
    return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
}

@end
