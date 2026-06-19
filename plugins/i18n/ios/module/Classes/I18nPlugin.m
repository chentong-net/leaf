#import "I18nPlugin.h"

static const int32_t REQUEST_ERROR_INTERNAL = -500;
static const int32_t REQUEST_ERROR_METHOD_NOT_IMPLEMENTED = -404;

@implementation I18nPlugin

- (NSString *)pluginName {
    return @"I18nPlugin";
}

- (BOOL)canHandle:(NSString *)method {
    return [method isEqualToString:@"i18n.get_system_language"];
}

- (void)onMethodCall:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSString *method = call.method ?: @"";
    if ([method isEqualToString:@"i18n.get_system_language"]) {
        [self handleGetSystemLanguage:call result:result];
        return;
    }
    [result error:call.requestId
             code:REQUEST_ERROR_METHOD_NOT_IMPLEMENTED
            error:@"method_not_implemented"
         canceled:NO];
}

- (void)handleGetSystemLanguage:(id<LeafMethodCall>)call result:(id<LeafResult>)result {
    NSString *languageTag = [NSLocale preferredLanguages].firstObject;
    if (languageTag.length == 0) {
        languageTag = @"en";
    }
    NSDictionary *payload = @{@"languageTag": languageTag};
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
