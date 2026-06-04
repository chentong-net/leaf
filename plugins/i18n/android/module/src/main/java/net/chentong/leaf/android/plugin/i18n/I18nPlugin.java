package net.chentong.leaf.android.plugin.i18n;

import android.content.Context;
import android.content.res.Configuration;
import android.os.Build;

import net.chentong.leaf.android.plugin.LeafPlugin;

import org.json.JSONObject;

import java.util.Locale;

public class I18nPlugin implements LeafPlugin {
    private final Context context;

    public I18nPlugin(Context context) {
        this.context = context == null ? null : context.getApplicationContext();
    }

    @Override
    public String pluginName() {
        return "I18nPlugin";
    }

    @Override
    public boolean canHandle(String method) {
        return "i18n.get_system_language".equals(method);
    }

    @Override
    public void onMethodCall(LeafMethodCall call, Result result) {
        if (!"i18n.get_system_language".equals(call.getMethod())) {
            result.error(call.getRequestId(), -404, "method_not_implemented", false);
            return;
        }

        try {
            final String languageTag = resolveLanguageTag();
            if (languageTag.isEmpty()) {
                result.error(call.getRequestId(), -2, "system_language_empty", false);
                return;
            }

            JSONObject out = new JSONObject();
            out.put("languageTag", languageTag);
            result.success(call.getRequestId(), out.toString());
        } catch (Throwable throwable) {
            result.error(call.getRequestId(), -500, "get_system_language_failed", false);
        }
    }

    private String resolveLanguageTag() {
        Locale locale = null;

        if (context != null) {
            Configuration configuration = context.getResources().getConfiguration();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                if (!configuration.getLocales().isEmpty()) {
                    locale = configuration.getLocales().get(0);
                }
            } else {
                locale = configuration.locale;
            }
        }

        if (locale == null) {
            locale = Locale.getDefault();
        }
        if (locale == null) {
            return "";
        }

        String tag = locale.toLanguageTag();
        return tag == null ? "" : tag;
    }
}
