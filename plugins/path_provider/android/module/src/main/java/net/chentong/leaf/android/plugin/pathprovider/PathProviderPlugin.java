package net.chentong.leaf.android.plugin.pathprovider;

import android.content.Context;
import android.os.Build;
import android.os.Environment;

import net.chentong.leaf.android.plugin.LeafPlugin;

import org.json.JSONObject;

import java.io.File;

public class PathProviderPlugin implements LeafPlugin {
    private final Context context;

    public PathProviderPlugin(Context context) {
        this.context = context;
    }

    @Override
    public String pluginName() {
        return "PathProviderPlugin";
    }

    @Override
    public boolean canHandle(String method) {
        return "path_provider.get_temporary_path".equals(method)
            || "path_provider.get_application_support_path".equals(method)
            || "path_provider.get_application_documents_path".equals(method)
            || "path_provider.get_downloads_path".equals(method)
            || "path_provider.get_external_storage_path".equals(method);
    }

    @Override
    public void onMethodCall(LeafMethodCall call, Result result) {
        String method = call.getMethod();
        if ("path_provider.get_temporary_path".equals(method)) {
            emitPath(call, result, pathOf(context.getCacheDir()));
            return;
        }
        if ("path_provider.get_application_support_path".equals(method)) {
            emitPath(call, result, pathOf(context.getFilesDir()));
            return;
        }
        if ("path_provider.get_application_documents_path".equals(method)) {
            File documentsDir = context.getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS);
            if (documentsDir == null) {
                documentsDir = context.getFilesDir();
            }
            emitPath(call, result, pathOf(documentsDir));
            return;
        }
        if ("path_provider.get_downloads_path".equals(method)) {
            String path = "";
            File downloadsDir = context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
            if (downloadsDir != null) {
                path = pathOf(downloadsDir);
            } else if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
                path = pathOf(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS));
            }
            emitPath(call, result, path);
            return;
        }
        if ("path_provider.get_external_storage_path".equals(method)) {
            emitPath(call, result, pathOf(context.getExternalFilesDir(null)));
            return;
        }
        result.error(call.getRequestId(), -404, "method_not_implemented", false);
    }

    private void emitPath(LeafMethodCall call, Result result, String path) {
        try {
            JSONObject out = new JSONObject();
            out.put("path", path == null ? "" : path);
            result.success(call.getRequestId(), out.toString());
        } catch (Throwable throwable) {
            result.error(call.getRequestId(), -500, "serialize_result_failed", false);
        }
    }

    private String pathOf(File dir) {
        if (dir == null) {
            return "";
        }
        return dir.getAbsolutePath();
    }
}
