package net.chentong.leaf.android.plugin.filepicker;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;

import net.chentong.leaf.android.plugin.LeafPlugin;

import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;

public class FilePickerPlugin implements LeafPlugin {
    private static final int REQUEST_CODE_PICK = 0x1107;

    private final Activity activity;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final AtomicLong fileIdGenerator = new AtomicLong(1);
    private final Map<String, PickedFileRecord> pickedFiles = new ConcurrentHashMap<>();
    private volatile PendingPick pendingPick;

    public FilePickerPlugin(Activity activity) {
        this.activity = activity;
    }

    @Override
    public String pluginName() {
        return "FilePickerPlugin";
    }

    @Override
    public boolean canHandle(String method) {
        return "file_picker.pick".equals(method) || "file_picker.open_fd".equals(method);
    }

    @Override
    public void onMethodCall(LeafMethodCall call, Result result) {
        if ("file_picker.pick".equals(call.getMethod())) {
            handlePick(call, result);
            return;
        }
        if ("file_picker.open_fd".equals(call.getMethod())) {
            handleOpenFd(call, result);
            return;
        }
        result.error(call.getRequestId(), -404, "method_not_implemented", false);
    }

    public boolean onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode != REQUEST_CODE_PICK) {
            return false;
        }

        PendingPick pending = pendingPick;
        pendingPick = null;
        if (pending == null) {
            return true;
        }

        if (resultCode != Activity.RESULT_OK || data == null || data.getData() == null) {
            pending.result.error(pending.requestId, 0, "canceled", true);
            return true;
        }

        Uri uri = data.getData();
        try {
            final int grantedFlags = data.getFlags()
                & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            try {
                activity.getContentResolver().takePersistableUriPermission(uri, grantedFlags);
            } catch (Throwable ignored) {
                // Some sources may not support persistable permissions.
            }

            String displayName = queryDisplayName(uri);
            if (displayName == null || displayName.isEmpty()) {
                displayName = "picked_file";
            }
            long size = querySize(uri);
            String mimeType = activity.getContentResolver().getType(uri);
            if (mimeType == null) {
                mimeType = "";
            }

            String fileId = "fp_" + fileIdGenerator.getAndIncrement();
            String sandboxPath = "";
            if (pending.copyToSandbox) {
                sandboxPath = copyToSandbox(uri, displayName);
            }

            PickedFileRecord record = new PickedFileRecord();
            record.fileId = fileId;
            record.uri = uri.toString();
            record.name = displayName;
            record.mimeType = mimeType;
            record.path = sandboxPath;
            record.size = size;
            pickedFiles.put(fileId, record);

            JSONObject out = new JSONObject();
            out.put("fileId", fileId);
            out.put("path", sandboxPath);
            out.put("name", displayName);
            out.put("mimeType", mimeType);
            out.put("size", size);
            pending.result.success(pending.requestId, out.toString());
        } catch (Throwable throwable) {
            pending.result.error(pending.requestId, -2, "pick_failed", false);
        }
        return true;
    }

    private void handlePick(LeafMethodCall call, Result result) {
        if (activity == null) {
            result.error(call.getRequestId(), -10, "activity_unavailable", false);
            return;
        }

        if (pendingPick != null) {
            result.error(call.getRequestId(), -11, "pick_in_progress", false);
            return;
        }

        final PickOptions options;
        try {
            options = parsePickOptions(call.getArgs());
        } catch (Throwable throwable) {
            result.error(call.getRequestId(), -1, "invalid_args", false);
            return;
        }

        PendingPick pending = new PendingPick();
        pending.requestId = call.getRequestId();
        pending.copyToSandbox = options.copyToSandbox;
        pending.result = result;
        pendingPick = pending;

        mainHandler.post(() -> {
            try {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, false);
                applyMimeType(intent, options.mediaType);
                activity.startActivityForResult(intent, REQUEST_CODE_PICK);
            } catch (Throwable throwable) {
                PendingPick failed = pendingPick;
                pendingPick = null;
                if (failed != null) {
                    failed.result.error(failed.requestId, -12, "open_picker_failed", false);
                }
            }
        });
    }

    private void handleOpenFd(LeafMethodCall call, Result result) {
        final String fileId;
        try {
            JSONObject json = parseArgs(call.getArgs());
            fileId = json.optString("fileId", "");
        } catch (Throwable throwable) {
            result.error(call.getRequestId(), -1, "invalid_args", false);
            return;
        }

        if (fileId.isEmpty()) {
            result.error(call.getRequestId(), -1, "invalid_args", false);
            return;
        }

        PickedFileRecord record = pickedFiles.get(fileId);
        if (record == null || record.uri == null || record.uri.isEmpty()) {
            result.error(call.getRequestId(), -404, "file_not_found", false);
            return;
        }

        try {
            Uri uri = Uri.parse(record.uri);
            ParcelFileDescriptor pfd = activity.getContentResolver().openFileDescriptor(uri, "r");
            if (pfd == null) {
                result.error(call.getRequestId(), -20, "read_source_unavailable", false);
                return;
            }
            int fd = pfd.detachFd();
            pfd.close();

            JSONObject out = new JSONObject();
            out.put("fileId", fileId);
            out.put("fd", fd);
            out.put("path", record.path == null ? "" : record.path);
            result.success(call.getRequestId(), out.toString());
        } catch (Throwable throwable) {
            result.error(call.getRequestId(), -20, "open_fd_failed", false);
        }
    }

    private PickOptions parsePickOptions(String args) throws Exception {
        JSONObject json = parseArgs(args);
        PickOptions options = new PickOptions();
        options.mediaType = json.optInt("mediaType", 0);
        options.copyToSandbox = json.optBoolean("copyToSandbox", true);
        return options;
    }

    private JSONObject parseArgs(String args) throws Exception {
        if (args == null || args.trim().isEmpty()) {
            return new JSONObject();
        }
        return new JSONObject(args);
    }

    private void applyMimeType(Intent intent, int mediaType) {
        switch (mediaType) {
            case 1:
                intent.setType("image/*");
                break;
            case 2:
                intent.setType("video/*");
                break;
            case 3:
                intent.setType("*/*");
                intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"image/*", "video/*"});
                break;
            default:
                intent.setType("*/*");
                break;
        }
    }

    private String queryDisplayName(Uri uri) {
        try (Cursor cursor = activity.getContentResolver().query(uri, null, null, null, null)) {
            if (cursor == null || !cursor.moveToFirst()) {
                return null;
            }
            int idx = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
            if (idx < 0) {
                return null;
            }
            return cursor.getString(idx);
        } catch (Throwable ignored) {
            return null;
        }
    }

    private long querySize(Uri uri) {
        try (Cursor cursor = activity.getContentResolver().query(uri, null, null, null, null)) {
            if (cursor == null || !cursor.moveToFirst()) {
                return 0L;
            }
            int idx = cursor.getColumnIndex(OpenableColumns.SIZE);
            if (idx < 0) {
                return 0L;
            }
            return cursor.getLong(idx);
        } catch (Throwable ignored) {
            return 0L;
        }
    }

    private String copyToSandbox(Uri uri, String fileName) throws Exception {
        File targetDir = new File(activity.getCacheDir(), "leaf/file_picker");
        if (!targetDir.exists() && !targetDir.mkdirs()) {
            throw new IllegalStateException("mkdir_failed");
        }

        String safeName = sanitizeFileName(fileName);
        if (safeName.isEmpty()) {
            safeName = "picked_file";
        }
        File target = new File(targetDir, System.currentTimeMillis() + "_" + safeName);

        try (InputStream in = activity.getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(target)) {
            if (in == null) {
                throw new IllegalStateException("open_input_failed");
            }

            byte[] buffer = new byte[8192];
            int read;
            while ((read = in.read(buffer)) >= 0) {
                if (read > 0) {
                    out.write(buffer, 0, read);
                }
            }
            out.flush();
            return target.getAbsolutePath();
        }
    }

    private String sanitizeFileName(String name) {
        if (name == null) return "";
        return name.replaceAll("[\\\\/:*?\"<>|]", "_");
    }

    private static final class PickOptions {
        int mediaType;
        boolean copyToSandbox;
    }

    private static final class PendingPick {
        int requestId;
        boolean copyToSandbox;
        Result result;
    }

    private static final class PickedFileRecord {
        String fileId;
        String uri;
        String name;
        String mimeType;
        String path;
        long size;
    }
}
