#include "LFLocalTimePlatform.h"

#include "LFJSONParser.h"
#include "plugin/LFNativeSender.h"

void LFLocalTimePlatform::getTimezone(LFLocalTimePlatformTimezoneCallback callback) {
    if (!callback) {
        return;
    }

    LFNativeSender::getInstance().send(
        "local_time.get_timezone",
        "{}",
        [callback = std::move(callback)](const LFMethodResult& result) mutable {
            LFLocalTimePlatformTimezoneResult out;
            out.ok = false;

            if (!result.ok) {
                out.error = result.error.empty() ? "get_timezone_failed" : result.error;
                callback(out);
                return;
            }

            try {
                auto json = LFJSONParser::parse(result.data);
                if (json && json->contains("timezone")) {
                    out.timezone = json->at("timezone").asString();
                }
                out.ok = !out.timezone.empty();
                if (!out.ok) {
                    out.error = "timezone_empty";
                }
            } catch (...) {
                out.error = "timezone_parse_failed";
            }

            callback(out);
        }
    );
}
