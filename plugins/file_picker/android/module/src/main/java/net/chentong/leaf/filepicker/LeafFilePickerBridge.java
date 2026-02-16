package net.chentong.leaf.filepicker;

import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;

import java.util.List;

public final class LeafFilePickerBridge {

    static final String EXTRA_REQUEST_ID = "leaf_file_picker_request_id";
    static final String EXTRA_MEDIA_TYPE = "leaf_file_picker_media_type";
    static final String EXTRA_COPY_TO_SANDBOX = "leaf_file_picker_copy_to_sandbox";

    private static final Handler MAIN_HANDLER = new Handler(Looper.getMainLooper());
    private static final Object LOCK = new Object();

    private static volatile Context appContext;
    private static int pendingRequestId = -1;
    private static int pendingMediaType = 0;
    private static boolean pendingCopyToSandbox = true;

    private LeafFilePickerBridge() {}

    public static void initialize(Context context) {
        if (context != null) {
            appContext = context.getApplicationContext();
        }
    }

    public static void openFilePicker(int requestId, int mediaType, boolean copyToSandbox) {
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
                pendingMediaType = mediaType;
                pendingCopyToSandbox = copyToSandbox;
            }

            try {
                Intent intent = new Intent(context, LeafFilePickerProxyActivity.class);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                intent.putExtra(EXTRA_REQUEST_ID, requestId);
                intent.putExtra(EXTRA_MEDIA_TYPE, mediaType);
                intent.putExtra(EXTRA_COPY_TO_SANDBOX, copyToSandbox);
                context.startActivity(intent);
            } catch (Throwable t) {
                synchronized (LOCK) {
                    if (pendingRequestId == requestId) {
                        pendingRequestId = -1;
                        pendingMediaType = 0;
                        pendingCopyToSandbox = true;
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

    static int readMediaType(Intent intent) {
        if (intent == null) {
            return pendingMediaType;
        }
        return intent.getIntExtra(EXTRA_MEDIA_TYPE, pendingMediaType);
    }

    static boolean readCopyToSandbox(Intent intent) {
        if (intent == null) {
            return pendingCopyToSandbox;
        }
        return intent.getBooleanExtra(EXTRA_COPY_TO_SANDBOX, pendingCopyToSandbox);
    }

    static void deliverResult(int requestId, boolean success, List<String> paths, String error) {
        int callbackRequestId = requestId;

        synchronized (LOCK) {
            if (pendingRequestId == callbackRequestId) {
                pendingRequestId = -1;
                pendingMediaType = 0;
                pendingCopyToSandbox = true;
            } else if (pendingRequestId >= 0 && callbackRequestId < 0) {
                callbackRequestId = pendingRequestId;
                pendingRequestId = -1;
                pendingMediaType = 0;
                pendingCopyToSandbox = true;
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
