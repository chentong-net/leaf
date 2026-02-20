#import "FilePickerPlugin.h"
#import <TargetConditionals.h>

#include <fcntl.h>
#include <unistd.h>

static const int32_t REQUEST_ERROR_INVALID_ARGS = -1;
static const int32_t REQUEST_ERROR_PICK_FAILED = -2;
static const int32_t REQUEST_ERROR_CONTEXT_UNAVAILABLE = -10;
static const int32_t REQUEST_ERROR_PICK_IN_PROGRESS = -11;
static const int32_t REQUEST_ERROR_OPEN_PICKER_FAILED = -12;
static const int32_t REQUEST_ERROR_READ_SOURCE_UNAVAILABLE = -20;
static const int32_t REQUEST_ERROR_FILE_NOT_FOUND = -404;

@interface FilePickerPendingPick : NSObject
@property (nonatomic, assign) int32_t requestId;
@property (nonatomic, assign) BOOL copyToSandbox;
@property (nonatomic, strong) id<LeafResult> result;
@end

@implementation FilePickerPendingPick
@end

@interface FilePickerRecord : NSObject
@property (nonatomic, copy) NSString *fileId;
@property (nonatomic, strong, nullable) NSURL *sourceURL;
@property (nonatomic, strong, nullable) NSData *bookmarkData;
@property (nonatomic, copy) NSString *name;
@property (nonatomic, copy) NSString *mimeType;
@property (nonatomic, copy) NSString *path;
@property (nonatomic, assign) long long size;
@end

@implementation FilePickerRecord
@end

@interface FilePickerPlugin ()
@property (nonatomic, weak) UIViewController *presentingViewController;
@property (nonatomic, strong) NSMutableDictionary<NSString *, FilePickerRecord *> *pickedFiles;
@property (nonatomic, strong, nullable) FilePickerPendingPick *pendingPick;
@property (nonatomic, assign) long long nextFileId;
@end

@implementation FilePickerPlugin

- (instancetype)initWithPresentingViewController:(UIViewController *)presentingViewController {
    self = [super init];
    if (self) {
        _presentingViewController = presentingViewController;
        _pickedFiles = [NSMutableDictionary dictionary];
        _nextFileId = 1;
    }
    return self;
}

- (NSString *)pluginName {
    return @"FilePickerPlugin";
}

- (BOOL)canHandle:(NSString *)method {
    return [method isEqualToString:@"file_picker.pick"] || [method isEqualToString:@"file_picker.open_fd"];
}

- (void)onMethodCall:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self onMethodCall:call result:result];
        });
        return;
    }

    if ([call.method isEqualToString:@"file_picker.pick"]) {
        [self handlePick:call result:result];
        return;
    }
    if ([call.method isEqualToString:@"file_picker.open_fd"]) {
        [self handleOpenFd:call result:result];
        return;
    }
    [result error:call.requestId code:REQUEST_ERROR_FILE_NOT_FOUND error:@"method_not_implemented" canceled:NO];
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {
    FilePickerPendingPick *pending = self.pendingPick;
    self.pendingPick = nil;
    if (!pending) {
        return;
    }
    [pending.result error:pending.requestId code:0 error:@"canceled" canceled:YES];
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentAtURL:(NSURL *)url {
    if (!url) {
        [self documentPickerWasCancelled:controller];
        return;
    }
    [self documentPicker:controller didPickDocumentsAtURLs:@[url]];
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    FilePickerPendingPick *pending = self.pendingPick;
    self.pendingPick = nil;
    if (!pending) {
        return;
    }

    NSURL *url = urls.firstObject;
    if (!url) {
        [pending.result error:pending.requestId code:0 error:@"canceled" canceled:YES];
        return;
    }

    NSError *pickError = nil;
    NSDictionary *payload = [self buildPickPayloadWithURL:url copyToSandbox:pending.copyToSandbox error:&pickError];
    if (!payload) {
        [pending.result error:pending.requestId code:REQUEST_ERROR_PICK_FAILED error:@"pick_failed" canceled:NO];
        return;
    }

    NSString *json = [self jsonStringFromDictionary:payload];
    if (!json) {
        [pending.result error:pending.requestId code:REQUEST_ERROR_PICK_FAILED error:@"pick_result_serialize_failed" canceled:NO];
        return;
    }
    [pending.result success:pending.requestId data:json];
}

- (void)handlePick:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    if (self.pendingPick != nil) {
        [result error:call.requestId code:REQUEST_ERROR_PICK_IN_PROGRESS error:@"pick_in_progress" canceled:NO];
        return;
    }

    NSDictionary *args = [self parseArgs:call.args];
    if (!args && call.args.length > 0) {
        [result error:call.requestId code:REQUEST_ERROR_INVALID_ARGS error:@"invalid_args" canceled:NO];
        return;
    }

    BOOL copyToSandbox = YES;
    NSNumber *copyValue = args[@"copyToSandbox"];
    if ([copyValue isKindOfClass:[NSNumber class]]) {
        copyToSandbox = copyValue.boolValue;
    }

    NSNumber *mediaTypeValue = args[@"mediaType"];
    NSInteger mediaType = 0;
    if ([mediaTypeValue isKindOfClass:[NSNumber class]]) {
        mediaType = mediaTypeValue.integerValue;
    }

    UIViewController *presenter = [self resolvePresenter];
    if (!presenter) {
        [result error:call.requestId code:REQUEST_ERROR_CONTEXT_UNAVAILABLE error:@"context_unavailable" canceled:NO];
        return;
    }

    FilePickerPendingPick *pending = [[FilePickerPendingPick alloc] init];
    pending.requestId = call.requestId;
    pending.copyToSandbox = copyToSandbox;
    pending.result = result;
    self.pendingPick = pending;

    UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:[self documentTypesForMediaType:mediaType]
                                                                                                      inMode:UIDocumentPickerModeOpen];
    picker.delegate = self;
    picker.allowsMultipleSelection = NO;

    @try {
        [presenter presentViewController:picker animated:YES completion:nil];
    } @catch (__unused NSException *exception) {
        self.pendingPick = nil;
        [result error:call.requestId code:REQUEST_ERROR_OPEN_PICKER_FAILED error:@"open_picker_failed" canceled:NO];
    }
}

- (void)handleOpenFd:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSDictionary *args = [self parseArgs:call.args];
    if (!args && call.args.length > 0) {
        [result error:call.requestId code:REQUEST_ERROR_INVALID_ARGS error:@"invalid_args" canceled:NO];
        return;
    }

    NSString *fileId = [args[@"fileId"] isKindOfClass:[NSString class]] ? args[@"fileId"] : @"";
    if (fileId.length == 0) {
        [result error:call.requestId code:REQUEST_ERROR_INVALID_ARGS error:@"invalid_args" canceled:NO];
        return;
    }

    FilePickerRecord *record = self.pickedFiles[fileId];
    if (!record) {
        [result error:call.requestId code:REQUEST_ERROR_FILE_NOT_FOUND error:@"file_not_found" canceled:NO];
        return;
    }

    int fd = -1;
    if (record.path.length > 0) {
        fd = open(record.path.fileSystemRepresentation, O_RDONLY);
    } else {
        NSError *error = nil;
        NSURL *url = [self resolveAccessibleURLForRecord:record error:&error];
        if (!url) {
            [result error:call.requestId code:REQUEST_ERROR_READ_SOURCE_UNAVAILABLE error:@"read_source_unavailable" canceled:NO];
            return;
        }

        BOOL accessing = [url startAccessingSecurityScopedResource];
        @try {
            NSFileHandle *handle = [NSFileHandle fileHandleForReadingFromURL:url error:&error];
            if (!handle) {
                [result error:call.requestId code:REQUEST_ERROR_READ_SOURCE_UNAVAILABLE error:@"open_fd_failed" canceled:NO];
                return;
            }
            fd = dup((int)handle.fileDescriptor);
            [handle closeFile];
        } @finally {
            if (accessing) {
                [url stopAccessingSecurityScopedResource];
            }
        }
    }

    if (fd < 0) {
        [result error:call.requestId code:REQUEST_ERROR_READ_SOURCE_UNAVAILABLE error:@"open_fd_failed" canceled:NO];
        return;
    }

    NSDictionary *payload = @{
        @"fileId": record.fileId ?: @"",
        @"fd": @(fd),
        @"path": record.path ?: @""
    };
    NSString *json = [self jsonStringFromDictionary:payload];
    if (!json) {
        close(fd);
        [result error:call.requestId code:REQUEST_ERROR_READ_SOURCE_UNAVAILABLE error:@"open_fd_result_serialize_failed" canceled:NO];
        return;
    }
    [result success:call.requestId data:json];
}

- (NSDictionary *)buildPickPayloadWithURL:(NSURL *)url copyToSandbox:(BOOL)copyToSandbox error:(NSError **)error {
    BOOL accessing = [url startAccessingSecurityScopedResource];
    @try {
        NSString *name = [self deriveNameFromURL:url];
        NSString *mimeType = [self deriveMimeTypeFromName:name];
        NSString *path = @"";
        if (copyToSandbox) {
            path = [self copyToSandboxFromURL:url displayName:name error:error];
            if (!path) {
                return nil;
            }
        }

        long long size = [self querySizeWithSandboxPath:path sourceURL:url];
        NSString *fileId = [self nextFileIdString];

        FilePickerRecord *record = [[FilePickerRecord alloc] init];
        record.fileId = fileId;
        record.sourceURL = url;
        record.name = name;
        record.mimeType = mimeType;
        record.path = path ?: @"";
        record.size = size;
        NSError *bookmarkError = nil;
#if TARGET_OS_MACCATALYST
        record.bookmarkData = [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
                             includingResourceValuesForKeys:nil
                                              relativeToURL:nil
                                                      error:&bookmarkError];
#else
        record.bookmarkData = [url bookmarkDataWithOptions:0
                             includingResourceValuesForKeys:nil
                                              relativeToURL:nil
                                                      error:&bookmarkError];
#endif
        self.pickedFiles[fileId] = record;

        return @{
            @"fileId": fileId,
            @"path": record.path,
            @"name": name,
            @"mimeType": mimeType,
            @"size": @(size)
        };
    } @finally {
        if (accessing) {
            [url stopAccessingSecurityScopedResource];
        }
    }
}

- (NSString *)copyToSandboxFromURL:(NSURL *)sourceURL displayName:(NSString *)displayName error:(NSError **)error {
    NSFileManager *fm = [NSFileManager defaultManager];
    NSArray<NSString *> *cacheDirs = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString *cacheRoot = cacheDirs.firstObject ?: NSTemporaryDirectory();
    NSString *targetDir = [cacheRoot stringByAppendingPathComponent:@"leaf/file_picker"];
    if (![fm createDirectoryAtPath:targetDir withIntermediateDirectories:YES attributes:nil error:error]) {
        return nil;
    }

    NSString *safeName = [self sanitizeFileName:displayName];
    if (safeName.length == 0) {
        safeName = @"picked_file";
    }
    NSString *targetName = [NSString stringWithFormat:@"%.0f_%@", [[NSDate date] timeIntervalSince1970] * 1000.0, safeName];
    NSString *targetPath = [targetDir stringByAppendingPathComponent:targetName];
    NSURL *targetURL = [NSURL fileURLWithPath:targetPath];

    NSFileHandle *input = [NSFileHandle fileHandleForReadingFromURL:sourceURL error:error];
    if (!input) {
        return nil;
    }
    if (![fm createFileAtPath:targetPath contents:nil attributes:nil]) {
        [input closeFile];
        if (error) {
            *error = [NSError errorWithDomain:@"FilePickerPlugin" code:-1 userInfo:nil];
        }
        return nil;
    }
    NSFileHandle *output = [NSFileHandle fileHandleForWritingToURL:targetURL error:error];
    if (!output) {
        [input closeFile];
        return nil;
    }

    @try {
        while (true) {
            NSData *chunk = [input readDataOfLength:8192];
            if (chunk.length == 0) {
                break;
            }
            [output writeData:chunk];
        }
    } @catch (__unused NSException *exception) {
        [input closeFile];
        [output closeFile];
        if (error) {
            *error = [NSError errorWithDomain:@"FilePickerPlugin" code:-1 userInfo:nil];
        }
        return nil;
    }

    [input closeFile];
    [output closeFile];
    return targetPath;
}

- (NSURL *)resolveAccessibleURLForRecord:(FilePickerRecord *)record error:(NSError **)error {
    if (record.bookmarkData.length > 0) {
        BOOL stale = NO;
        NSURLBookmarkResolutionOptions options = 0;
#if TARGET_OS_MACCATALYST
        options = NSURLBookmarkResolutionWithSecurityScope;
#endif
        NSURL *resolved = [NSURL URLByResolvingBookmarkData:record.bookmarkData
                                                    options:options
                                              relativeToURL:nil
                                        bookmarkDataIsStale:&stale
                                                      error:error];
        if (resolved) {
            return resolved;
        }
    }
    return record.sourceURL;
}

- (UIViewController *)resolvePresenter {
    UIViewController *base = self.presentingViewController;
    if (!base) {
        if (@available(iOS 13.0, *)) {
            for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
                if (![scene isKindOfClass:[UIWindowScene class]]) {
                    continue;
                }
                UIWindowScene *windowScene = (UIWindowScene *)scene;
                for (UIWindow *window in windowScene.windows) {
                    if (window.isKeyWindow && window.rootViewController) {
                        base = window.rootViewController;
                        break;
                    }
                }
                if (base) {
                    break;
                }
            }
        } else {
            base = UIApplication.sharedApplication.keyWindow.rootViewController;
        }
    }

    while (base.presentedViewController) {
        base = base.presentedViewController;
    }
    return base;
}

- (NSArray<NSString *> *)documentTypesForMediaType:(NSInteger)mediaType {
    switch (mediaType) {
        case 1:
            return @[@"public.image"];
        case 2:
            return @[@"public.movie"];
        case 3:
            return @[@"public.image", @"public.movie"];
        default:
            return @[@"public.item"];
    }
}

- (NSDictionary *)parseArgs:(NSString *)args {
    if (args.length == 0) {
        return @{};
    }
    NSData *data = [args dataUsingEncoding:NSUTF8StringEncoding];
    if (!data) {
        return nil;
    }
    id json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    if (![json isKindOfClass:[NSDictionary class]]) {
        return nil;
    }
    return (NSDictionary *)json;
}

- (NSString *)jsonStringFromDictionary:(NSDictionary *)dictionary {
    NSError *error = nil;
    NSData *data = [NSJSONSerialization dataWithJSONObject:dictionary options:0 error:&error];
    if (!data || error) {
        return nil;
    }
    return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
}

- (NSString *)nextFileIdString {
    NSString *fileId = [NSString stringWithFormat:@"fp_%lld", self.nextFileId];
    self.nextFileId += 1;
    return fileId;
}

- (NSString *)sanitizeFileName:(NSString *)name {
    if (name.length == 0) {
        return @"";
    }
    NSCharacterSet *invalid = [NSCharacterSet characterSetWithCharactersInString:@"\\/:*?\"<>|"];
    NSMutableString *result = [NSMutableString stringWithCapacity:name.length];
    for (NSUInteger i = 0; i < name.length; i++) {
        unichar ch = [name characterAtIndex:i];
        if ([invalid characterIsMember:ch]) {
            [result appendString:@"_"];
        } else {
            [result appendFormat:@"%C", ch];
        }
    }
    return result;
}

- (NSString *)deriveNameFromURL:(NSURL *)url {
    NSString *name = nil;
    [url getResourceValue:&name forKey:NSURLNameKey error:nil];
    if (name.length > 0) {
        return name;
    }
    if (url.lastPathComponent.length > 0) {
        return url.lastPathComponent;
    }
    return @"picked_file";
}

- (long long)querySizeWithSandboxPath:(NSString *)sandboxPath sourceURL:(NSURL *)sourceURL {
    NSFileManager *fm = [NSFileManager defaultManager];
    if (sandboxPath.length > 0) {
        NSDictionary *attrs = [fm attributesOfItemAtPath:sandboxPath error:nil];
        NSNumber *size = attrs[NSFileSize];
        if (size != nil) {
            return size.longLongValue;
        }
    }

    NSNumber *resourceSize = nil;
    [sourceURL getResourceValue:&resourceSize forKey:NSURLFileSizeKey error:nil];
    if (resourceSize != nil) {
        return resourceSize.longLongValue;
    }
    return 0;
}

- (NSString *)deriveMimeTypeFromName:(NSString *)name {
    NSString *lower = name.lowercaseString;
    if ([lower hasSuffix:@".jpg"] || [lower hasSuffix:@".jpeg"]) return @"image/jpeg";
    if ([lower hasSuffix:@".png"]) return @"image/png";
    if ([lower hasSuffix:@".gif"]) return @"image/gif";
    if ([lower hasSuffix:@".webp"]) return @"image/webp";
    if ([lower hasSuffix:@".bmp"]) return @"image/bmp";
    if ([lower hasSuffix:@".heic"]) return @"image/heic";
    if ([lower hasSuffix:@".heif"]) return @"image/heif";
    if ([lower hasSuffix:@".mp4"]) return @"video/mp4";
    if ([lower hasSuffix:@".mov"]) return @"video/quicktime";
    if ([lower hasSuffix:@".mkv"]) return @"video/x-matroska";
    if ([lower hasSuffix:@".avi"]) return @"video/x-msvideo";
    if ([lower hasSuffix:@".m4v"]) return @"video/x-m4v";
    if ([lower hasSuffix:@".3gp"]) return @"video/3gpp";
    if ([lower hasSuffix:@".webm"]) return @"video/webm";
    return @"";
}

@end
