#import "AudioPlayerPlugin.h"
#import <AVFoundation/AVFoundation.h>

static const int32_t REQUEST_ERROR_INVALID_ARGS = -1;
static const int32_t REQUEST_ERROR_SOURCE_NOT_SET = -2;
static const int32_t REQUEST_ERROR_PLAYER_NOT_FOUND = -404;
static const int32_t REQUEST_ERROR_METHOD_NOT_IMPLEMENTED = -404;
static const int32_t REQUEST_ERROR_INTERNAL = -500;
static const int32_t REQUEST_ERROR_LISTEN_IN_PROGRESS = -11;

#pragma mark - AudioPlayerRecord

@interface AudioPlayerRecord : NSObject
@property (nonatomic, copy) NSString *playerId;
@property (nonatomic, copy) NSString *source;
@property (nonatomic, assign) BOOL looping;
@property (nonatomic, assign) float volume;
@property (nonatomic, strong, nullable) AVAudioPlayer *player;
@property (nonatomic, strong, nullable) id<LeafResult> pendingListener;
@property (nonatomic, assign) int32_t pendingListenerRequestId;
@property (nonatomic, strong) NSMutableArray<NSString *> *events;
@end

@implementation AudioPlayerRecord
- (instancetype)init {
    self = [super init];
    if (self) {
        _looping = NO;
        _volume = 1.0f;
        _events = [NSMutableArray array];
    }
    return self;
}
@end

#pragma mark - AudioPlayerPlugin

@interface AudioPlayerPlugin () <AVAudioPlayerDelegate>
@property (nonatomic, strong) NSMutableDictionary<NSString *, AudioPlayerRecord *> *players;
@end

@implementation AudioPlayerPlugin

- (instancetype)init {
    self = [super init];
    if (self) {
        _players = [NSMutableDictionary dictionary];
        [self configureAudioSession];
    }
    return self;
}

- (void)configureAudioSession {
    AVAudioSession *session = [AVAudioSession sharedInstance];
    NSError *error = nil;
    [session setCategory:AVAudioSessionCategoryPlayback error:&error];
    if (!error) {
        [session setActive:YES error:nil];
    }
}

#pragma mark - LeafPlugin

- (NSString *)pluginName {
    return @"AudioPlayerPlugin";
}

- (BOOL)canHandle:(NSString *)method {
    return [method hasPrefix:@"audio_player."];
}

- (void)onMethodCall:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self onMethodCall:call result:result];
        });
        return;
    }

    NSString *method = call.method ?: @"";
    @try {
        if ([method isEqualToString:@"audio_player.set_source"]) {
            [self handleSetSource:call result:result];
            return;
        }
        if ([method isEqualToString:@"audio_player.play"]) {
            [self handlePlay:call result:result];
            return;
        }
        if ([method isEqualToString:@"audio_player.pause"]) {
            [self handlePause:call result:result];
            return;
        }
        if ([method isEqualToString:@"audio_player.stop"]) {
            [self handleStop:call result:result];
            return;
        }
        if ([method isEqualToString:@"audio_player.seek"]) {
            [self handleSeek:call result:result];
            return;
        }
        if ([method isEqualToString:@"audio_player.set_looping"]) {
            [self handleSetLooping:call result:result];
            return;
        }
        if ([method isEqualToString:@"audio_player.set_volume"]) {
            [self handleSetVolume:call result:result];
            return;
        }
        if ([method isEqualToString:@"audio_player.get_duration"]) {
            [self handleGetDuration:call result:result];
            return;
        }
        if ([method isEqualToString:@"audio_player.get_position"]) {
            [self handleGetPosition:call result:result];
            return;
        }
        if ([method isEqualToString:@"audio_player.is_playing"]) {
            [self handleIsPlaying:call result:result];
            return;
        }
        if ([method isEqualToString:@"audio_player.listen"]) {
            [self handleListen:call result:result];
            return;
        }
        if ([method isEqualToString:@"audio_player.dispose"]) {
            [self handleDispose:call result:result];
            return;
        }
        [result error:call.requestId code:REQUEST_ERROR_METHOD_NOT_IMPLEMENTED error:@"method_not_implemented" canceled:NO];
    } @catch (NSException *exception) {
        NSString *error = exception.reason.length > 0 ? exception.reason : @"plugin_dispatch_throw";
        [result error:call.requestId code:REQUEST_ERROR_INTERNAL error:error canceled:NO];
    }
}

#pragma mark - Method handlers

- (void)handleSetSource:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSDictionary *args = [self parseArgs:call.args];
    NSString *playerId = [self requirePlayerId:args call:call result:result];
    if (!playerId) return;

    NSString *source = [args[@"source"] isKindOfClass:[NSString class]] ? args[@"source"] : @"";
    if (source.length == 0) {
        [result error:call.requestId code:REQUEST_ERROR_INVALID_ARGS error:@"source_empty" canceled:NO];
        return;
    }

    AudioPlayerRecord *record = [self getOrCreateRecord:playerId];
    [self releasePlayer:record];

    NSURL *url = [NSURL fileURLWithPath:source];
    NSError *error = nil;
    AVAudioPlayer *player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
    if (!player) {
        NSString *errorText = error.localizedDescription ?: @"audio_open_failed";
        [result error:call.requestId code:REQUEST_ERROR_INTERNAL error:errorText canceled:NO];
        return;
    }

    player.delegate = self;
    player.numberOfLoops = record.looping ? -1 : 0;
    player.volume = record.volume;
    [player prepareToPlay];

    record.source = source;
    record.player = player;
    [result success:call.requestId data:[self buildPlayerJson:playerId]];
}

- (void)handlePlay:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    AudioPlayerRecord *record = [self requireRecord:call result:result];
    if (!record) return;
    if (!record.player) {
        [result error:call.requestId code:REQUEST_ERROR_SOURCE_NOT_SET error:@"source_not_set" canceled:NO];
        return;
    }
    [record.player play];
    [result success:call.requestId data:[self buildPlayerJson:record.playerId]];
}

- (void)handlePause:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    AudioPlayerRecord *record = [self requireRecord:call result:result];
    if (!record) return;
    if (record.player && record.player.isPlaying) {
        [record.player pause];
    }
    [result success:call.requestId data:[self buildPlayerJson:record.playerId]];
}

- (void)handleStop:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    AudioPlayerRecord *record = [self requireRecord:call result:result];
    if (!record) return;
    if (record.player) {
        [record.player stop];
        record.player.currentTime = 0.0;
    }
    [result success:call.requestId data:[self buildPlayerJson:record.playerId]];
}

- (void)handleSeek:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSDictionary *args = [self parseArgs:call.args];
    AudioPlayerRecord *record = [self requireRecord:args call:call result:result];
    if (!record) return;
    double position = MAX(0.0, [args[@"position"] doubleValue]);
    if (record.player) {
        if (position > record.player.duration) {
            position = record.player.duration;
        }
        record.player.currentTime = position;
    }
    [result success:call.requestId data:[self buildPlayerJson:record.playerId]];
}

- (void)handleSetLooping:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSDictionary *args = [self parseArgs:call.args];
    AudioPlayerRecord *record = [self requireRecord:args call:call result:result];
    if (!record) return;
    BOOL looping = [args[@"looping"] boolValue];
    record.looping = looping;
    if (record.player) {
        record.player.numberOfLoops = looping ? -1 : 0;
    }
    [result success:call.requestId data:[self buildPlayerJson:record.playerId]];
}

- (void)handleSetVolume:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSDictionary *args = [self parseArgs:call.args];
    AudioPlayerRecord *record = [self requireRecord:args call:call result:result];
    if (!record) return;
    double volume = [args[@"volume"] doubleValue];
    if (volume < 0.0) volume = 0.0;
    if (volume > 1.0) volume = 1.0;
    record.volume = (float)volume;
    if (record.player) {
        record.player.volume = record.volume;
    }
    [result success:call.requestId data:[self buildPlayerJson:record.playerId]];
}

- (void)handleGetDuration:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    AudioPlayerRecord *record = [self requireRecord:call result:result];
    if (!record) return;
    double duration = record.player ? record.player.duration : 0.0;
    NSString *json = [self jsonStringFromDictionary:@{
        @"playerId": record.playerId ?: @"",
        @"duration": @(duration)
    }];
    if (!json) {
        [result error:call.requestId code:REQUEST_ERROR_INTERNAL error:@"serialize_result_failed" canceled:NO];
        return;
    }
    [result success:call.requestId data:json];
}

- (void)handleGetPosition:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    AudioPlayerRecord *record = [self requireRecord:call result:result];
    if (!record) return;
    double position = record.player ? record.player.currentTime : 0.0;
    NSString *json = [self jsonStringFromDictionary:@{
        @"playerId": record.playerId ?: @"",
        @"position": @(position)
    }];
    if (!json) {
        [result error:call.requestId code:REQUEST_ERROR_INTERNAL error:@"serialize_result_failed" canceled:NO];
        return;
    }
    [result success:call.requestId data:json];
}

- (void)handleIsPlaying:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    AudioPlayerRecord *record = [self requireRecord:call result:result];
    if (!record) return;
    BOOL playing = record.player ? record.player.isPlaying : NO;
    NSString *json = [self jsonStringFromDictionary:@{
        @"playerId": record.playerId ?: @"",
        @"playing": @(playing)
    }];
    if (!json) {
        [result error:call.requestId code:REQUEST_ERROR_INTERNAL error:@"serialize_result_failed" canceled:NO];
        return;
    }
    [result success:call.requestId data:json];
}

- (void)handleListen:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSDictionary *args = [self parseArgs:call.args];
    NSString *playerId = [self requirePlayerId:args call:call result:result];
    if (!playerId) return;

    AudioPlayerRecord *record = [self getOrCreateRecord:playerId];

    if (record.pendingListener != nil) {
        [result error:call.requestId code:REQUEST_ERROR_LISTEN_IN_PROGRESS error:@"listen_in_progress" canceled:NO];
        return;
    }

    if (record.events.count > 0) {
        NSString *event = record.events.firstObject;
        [record.events removeObjectAtIndex:0];
        [result success:call.requestId data:event];
        return;
    }

    record.pendingListener = result;
    record.pendingListenerRequestId = call.requestId;
}

- (void)handleDispose:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSDictionary *args = [self parseArgs:call.args];
    NSString *playerId = [self requirePlayerId:args call:call result:result];
    if (!playerId) return;

    AudioPlayerRecord *record = self.players[playerId];
    if (record) {
        [self releaseRecord:record];
        [self.players removeObjectForKey:playerId];
    }
    [result success:call.requestId data:[self buildPlayerJson:playerId]];
}

#pragma mark - AVAudioPlayerDelegate

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag {
    AudioPlayerRecord *record = [self recordForPlayer:player];
    if (!record) return;

    if (record.looping) {
        return;
    }

    if (!flag) {
        [self enqueueEvent:record type:@"error" error:@"audio_decode_error" code:-1];
        return;
    }

    double position = player.duration;
    [self enqueueEvent:record type:@"complete" error:@"" code:0 position:position];
}

- (void)audioPlayerDecodeErrorDidOccur:(AVAudioPlayer *)player error:(NSError *)error {
    AudioPlayerRecord *record = [self recordForPlayer:player];
    if (!record) return;
    NSString *errorText = error.localizedDescription ?: @"decode_error";
    [self enqueueEvent:record type:@"error" error:errorText code:(int32_t)error.code];
}

#pragma mark - Record management

- (AudioPlayerRecord *)getOrCreateRecord:(NSString *)playerId {
    AudioPlayerRecord *existing = self.players[playerId];
    if (existing) {
        return existing;
    }
    AudioPlayerRecord *record = [[AudioPlayerRecord alloc] init];
    record.playerId = playerId;
    self.players[playerId] = record;
    return record;
}

- (AudioPlayerRecord *)requireRecord:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    return [self requireRecord:[self parseArgs:call.args] call:call result:result];
}

- (AudioPlayerRecord *)requireRecord:(NSDictionary *)args call:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSString *playerId = [self requirePlayerId:args call:call result:result];
    if (!playerId) return nil;
    AudioPlayerRecord *record = self.players[playerId];
    if (!record) {
        [result error:call.requestId code:REQUEST_ERROR_PLAYER_NOT_FOUND error:@"player_not_found" canceled:NO];
        return nil;
    }
    return record;
}

- (AudioPlayerRecord *)recordForPlayer:(AVAudioPlayer *)player {
    for (AudioPlayerRecord *record in self.players.allValues) {
        if (record.player == player) {
            return record;
        }
    }
    return nil;
}

- (AudioPlayerRecord *)recordForPlayerId:(NSString *)playerId {
    AudioPlayerRecord *record = self.players[playerId];
    if (!record) return nil;
    return record;
}

#pragma mark - Event queue

- (void)enqueueEvent:(AudioPlayerRecord *)record
                type:(NSString *)type
               error:(NSString *)error
                code:(int32_t)code {
    [self enqueueEvent:record type:type error:error code:code position:(record.player ? record.player.currentTime : 0.0)];
}

- (void)enqueueEvent:(AudioPlayerRecord *)record
                type:(NSString *)type
               error:(NSString *)error
                code:(int32_t)code
            position:(double)position {
    if (!record) return;

    NSString *json = [self jsonStringFromDictionary:@{
        @"playerId": record.playerId ?: @"",
        @"type": type ?: @"",
        @"error": error ?: @"",
        @"code": @(code),
        @"position": @(position)
    }];
    if (!json) return;

    if (record.pendingListener != nil) {
        id<LeafResult> pending = record.pendingListener;
        int32_t pendingId = record.pendingListenerRequestId;
        record.pendingListener = nil;
        record.pendingListenerRequestId = 0;
        [pending success:pendingId data:json];
    } else {
        [record.events addObject:json];
    }
}

#pragma mark - Lifecycle

- (void)releasePlayer:(AudioPlayerRecord *)record {
    if (!record || !record.player) return;
    record.player.delegate = nil;
    [record.player stop];
    record.player = nil;
}

- (void)releaseRecord:(AudioPlayerRecord *)record {
    if (!record) return;
    [self releasePlayer:record];
    if (record.pendingListener != nil) {
        [record.pendingListener error:record.pendingListenerRequestId
                                 code:REQUEST_ERROR_INTERNAL
                                error:@"player_disposed"
                             canceled:YES];
        record.pendingListener = nil;
        record.pendingListenerRequestId = 0;
    }
    [record.events removeAllObjects];
}

#pragma mark - Helpers

- (NSString *)requirePlayerId:(NSDictionary *)args call:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSString *playerId = [args[@"playerId"] isKindOfClass:[NSString class]] ? args[@"playerId"] : @"";
    if (playerId.length == 0) {
        [result error:call.requestId code:REQUEST_ERROR_INVALID_ARGS error:@"player_id_empty" canceled:NO];
        return nil;
    }
    return playerId;
}

- (NSString *)buildPlayerJson:(NSString *)playerId {
    return [self jsonStringFromDictionary:@{
        @"playerId": playerId ?: @""
    }] ?: @"{}";
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

@end
