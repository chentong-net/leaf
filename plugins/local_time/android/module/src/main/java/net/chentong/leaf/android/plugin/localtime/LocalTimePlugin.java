package net.chentong.leaf.android.plugin.localtime;

import net.chentong.leaf.android.plugin.LeafPlugin;

import org.json.JSONObject;

import java.util.TimeZone;

public class LocalTimePlugin implements LeafPlugin {
    @Override
    public String pluginName() {
        return "LocalTimePlugin";
    }

    @Override
    public boolean canHandle(String method) {
        return "local_time.get_timezone".equals(method);
    }

    @Override
    public void onMethodCall(LeafMethodCall call, Result result) {
        if (!"local_time.get_timezone".equals(call.getMethod())) {
            result.error(call.getRequestId(), -404, "method_not_implemented", false);
            return;
        }

        try {
            final String timezone = resolveTimezone();
            if (timezone.isEmpty()) {
                result.error(call.getRequestId(), -2, "timezone_empty", false);
                return;
            }

            JSONObject out = new JSONObject();
            out.put("timezone", timezone);
            result.success(call.getRequestId(), out.toString());
        } catch (Throwable throwable) {
            result.error(call.getRequestId(), -500, "get_timezone_failed", false);
        }
    }

    private String resolveTimezone() {
        TimeZone timezone = TimeZone.getDefault();
        if (timezone == null) {
            return "";
        }

        String id = timezone.getID();
        return id == null ? "" : id;
    }
}
