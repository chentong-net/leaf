package net.chentong.leaf.filepicker;

import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;

import java.util.List;

public final class LeafFilePickerBridge {

    static final String EXTRA_REQUEST_ID = "leaf_file_picker_request_id";

    private static final Handler MAIN_HANDLER = new Handler(Looper.getMainLooper());
    private static final Object LOCK = new Object();

    private static volatile Context appContext;
    private static int pendingRequestId = -1;

    private LeafFilePickerBridge() {}

    public static void initialize(Context context) {
        if (context != null) {
            appContext = context.getApplicationContext();
        }
    }

    public static void openFilePicker(int requestId) {
        Context context = appContext;
        if (context == null) {
            nativeOnFilePickerResult(requestId, false, new String[0], "context_unavailable");
            return;
        }

        MAIN_HANDLER.post(() -> {
            synchronized (LOCK) {
                if (pendingRequestId != -1) {
                    nativeOnFilePickerResult(requestId, false, new String[0], "picker_busy");
                    return;
                }
                pendingRequestId = requestId;
            }

            try {
                Intent intent = new Intent(context, LeafFilePickerProxyActivity.class);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                intent.putExtra(EXTRA_REQUEST_ID, requestId);
                context.startActivity(intent);
            } catch (Throwable t) {
                synchronized (LOCK) {
                    if (pendingRequestId == requestId) {
                        pendingRequestId = -1;
                    }
                }
                nativeOnFilePickerResult(requestId, false, new String[0], "open_picker_failed");
            }
        });
    }

    static int readRequestId(Intent intent) {
        if (intent == null) {
            return -1;
        }
        return intent.getIntExtra(EXTRA_REQUEST_ID, -1);
    }

    static void deliverResult(int requestId, boolean success, List<String> paths, String error) {
        int callbackRequestId = requestId;

        synchronized (LOCK) {
            if (pendingRequestId == callbackRequestId) {
                pendingRequestId = -1;
            } else if (pendingRequestId >= 0 && callbackRequestId < 0) {
                callbackRequestId = pendingRequestId;
                pendingRequestId = -1;
            }
        }

        if (callbackRequestId < 0) {
            return;
        }

        if (!success) {
            nativeOnFilePickerResult(
                    callbackRequestId,
                    false,
                    new String[0],
                    TextUtils.isEmpty(error) ? "canceled" : error);
            return;
        }

        if (paths == null || paths.isEmpty()) {
            nativeOnFilePickerResult(callbackRequestId, false, new String[0], "empty_result");
            return;
        }

        nativeOnFilePickerResult(callbackRequestId, true, paths.toArray(new String[0]), "");
    }

    private static native void nativeOnFilePickerResult(
            int requestId,
            boolean success,
            String[] paths,
            String error
    );
}
