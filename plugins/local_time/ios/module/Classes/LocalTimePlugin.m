#import "LocalTimePlugin.h"

static const int32_t REQUEST_ERROR_INTERNAL = -500;
static const int32_t REQUEST_ERROR_METHOD_NOT_IMPLEMENTED = -404;

@implementation LocalTimePlugin

- (NSString *)pluginName {
    return @"LocalTimePlugin";
}

- (BOOL)canHandle:(NSString *)method {
    return [method isEqualToString:@"local_time.get_timezone"];
}

- (void)onMethodCall:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSString *method = call.method ?: @"";
    if ([method isEqualToString:@"local_time.get_timezone"]) {
        [self handleGetTimezone:call result:result];
        return;
    }
    [result error:call.requestId
             code:REQUEST_ERROR_METHOD_NOT_IMPLEMENTED
            error:@"method_not_implemented"
         canceled:NO];
}

- (void)handleGetTimezone:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSString *timezone = NSTimeZone.localTimeZone.name;
    if (timezone.length == 0) {
        [result error:call.requestId
                 code:REQUEST_ERROR_INTERNAL
                error:@"timezone_unavailable"
             canceled:NO];
        return;
    }
    NSDictionary *payload = @{@"timezone": timezone};
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

- (nullable NSString *)jsonStringFromDictionary:(NSDictionary *)dictionary {
    NSError *error = nil;
    NSData *data = [NSJSONSerialization dataWithJSONObject:dictionary options:0 error:&error];
    if (!data || error) {
        return nil;
    }
    return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
}

@end
