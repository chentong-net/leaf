package net.chentong.leaf.android;

import android.app.Activity;
import android.content.ClipData;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.util.Log;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;

public final class LeafPluginBridge {

    private static final String TAG = "LeafPluginBridge";
    private static final int REQUEST_FILE_PICKER = 0x4C50;

    private static WeakReference<Activity> activityRef = new WeakReference<>(null);
    private static int pendingRequestId = -1;

    private LeafPluginBridge() {}

    public static void bindActivity(Activity activity) {
        activityRef = new WeakReference<>(activity);
    }

    public static void openFilePicker(int requestId) {
        Activity activity = activityRef.get();
        if (activity == null) {
            nativeOnFilePickerResult(requestId, false, new String[0], "activity_not_bound");
            return;
        }

        activity.runOnUiThread(() -> {
            if (pendingRequestId != -1) {
                nativeOnFilePickerResult(requestId, false, new String[0], "picker_busy");
                return;
            }

            try {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("*/*");
                intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, false);
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);

                pendingRequestId = requestId;
                activity.startActivityForResult(Intent.createChooser(intent, "Select File"), REQUEST_FILE_PICKER);
            } catch (Throwable t) {
                pendingRequestId = -1;
                Log.e(TAG, "openFilePicker failed", t);
                nativeOnFilePickerResult(requestId, false, new String[0], "open_picker_failed");
            }
        });
    }

    public static boolean onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode != REQUEST_FILE_PICKER) {
            return false;
        }

        int requestId = pendingRequestId;
        pendingRequestId = -1;

        if (requestId < 0) {
            return true;
        }

        if (resultCode != Activity.RESULT_OK || data == null) {
            nativeOnFilePickerResult(requestId, false, new String[0], "canceled");
            return true;
        }

        List<String> paths = new ArrayList<>();
        Uri single = data.getData();
        if (single != null) {
            paths.add(single.toString());
        }

        ClipData clipData = data.getClipData();
        if (clipData != null) {
            for (int i = 0; i < clipData.getItemCount(); i++) {
                ClipData.Item item = clipData.getItemAt(i);
                Uri uri = item != null ? item.getUri() : null;
                if (uri != null) {
                    paths.add(uri.toString());
                }
            }
        }

        if (paths.isEmpty()) {
            nativeOnFilePickerResult(requestId, false, new String[0], "empty_result");
            return true;
        }

        Activity activity = activityRef.get();
        if (activity != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            for (String path : paths) {
                try {
                    Uri uri = Uri.parse(path);
                    activity.getContentResolver().takePersistableUriPermission(
                            uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
                } catch (Throwable ignored) {
                }
            }
        }

        nativeOnFilePickerResult(requestId, true, paths.toArray(new String[0]), "");
        return true;
    }

    private static native void nativeOnFilePickerResult(
            int requestId,
            boolean success,
            String[] paths,
            String error
    );
}
