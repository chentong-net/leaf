package net.chentong.leaf.filepicker;

import android.app.Activity;
import android.content.ClipData;
import android.content.ContentResolver;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.OpenableColumns;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

public final class LeafFilePickerProxyActivity extends Activity {

    private static final int REQUEST_FILE_PICKER = 0x4C50;
    private static final String STATE_REQUEST_ID = "leaf_file_picker_state_request_id";
    private static final String STATE_PICKER_OPENED = "leaf_file_picker_state_picker_opened";
    private static final String STATE_MEDIA_TYPE = "leaf_file_picker_state_media_type";
    private static final String STATE_COPY_TO_SANDBOX = "leaf_file_picker_state_copy_to_sandbox";

    private int requestId = -1;
    private boolean pickerOpened = false;
    private int mediaType = 0;
    private boolean copyToSandbox = true;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (savedInstanceState != null) {
            requestId = savedInstanceState.getInt(STATE_REQUEST_ID, -1);
            pickerOpened = savedInstanceState.getBoolean(STATE_PICKER_OPENED, false);
            mediaType = savedInstanceState.getInt(STATE_MEDIA_TYPE, 0);
            copyToSandbox = savedInstanceState.getBoolean(STATE_COPY_TO_SANDBOX, true);
        } else {
            requestId = LeafFilePickerBridge.readRequestId(getIntent());
            mediaType = LeafFilePickerBridge.readMediaType(getIntent());
            copyToSandbox = LeafFilePickerBridge.readCopyToSandbox(getIntent());
        }
        if (requestId < 0) {
            finish();
            return;
        }

        if (!pickerOpened) {
            openFilePicker();
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt(STATE_REQUEST_ID, requestId);
        outState.putBoolean(STATE_PICKER_OPENED, pickerOpened);
        outState.putInt(STATE_MEDIA_TYPE, mediaType);
        outState.putBoolean(STATE_COPY_TO_SANDBOX, copyToSandbox);
    }

    private void openFilePicker() {
        try {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            applyMimeFilter(intent);
            intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, false);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
            pickerOpened = true;
            startActivityForResult(Intent.createChooser(intent, "Select File"), REQUEST_FILE_PICKER);
        } catch (Throwable t) {
            LeafFilePickerBridge.deliverResult(requestId, false, null, "open_picker_failed");
            finish();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_FILE_PICKER) {
            return;
        }

        if (resultCode != Activity.RESULT_OK || data == null) {
            LeafFilePickerBridge.deliverResult(requestId, false, null, "canceled");
            finish();
            return;
        }

        List<Uri> pickedUris = new ArrayList<>();
        Uri single = data.getData();
        if (single != null) {
            pickedUris.add(single);
        }

        ClipData clipData = data.getClipData();
        if (clipData != null) {
            for (int i = 0; i < clipData.getItemCount(); i++) {
                ClipData.Item item = clipData.getItemAt(i);
                Uri uri = item != null ? item.getUri() : null;
                if (uri != null) {
                    pickedUris.add(uri);
                }
            }
        }

        if (pickedUris.isEmpty()) {
            LeafFilePickerBridge.deliverResult(requestId, false, null, "empty_result");
            finish();
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            for (Uri uri : pickedUris) {
                try {
                    getContentResolver().takePersistableUriPermission(
                            uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
                } catch (Throwable ignored) {
                }
            }
        }

        List<String> resultPaths = new ArrayList<>();
        if (copyToSandbox) {
            for (Uri uri : pickedUris) {
                String localPath = copyUriToLocalFile(uri);
                if (localPath != null && !localPath.isEmpty()) {
                    resultPaths.add(localPath);
                }
            }
            if (resultPaths.isEmpty()) {
                LeafFilePickerBridge.deliverResult(requestId, false, null, "copy_to_local_failed");
                finish();
                return;
            }
        } else {
            for (Uri uri : pickedUris) {
                if (uri != null) {
                    resultPaths.add(uri.toString());
                }
            }
            if (resultPaths.isEmpty()) {
                LeafFilePickerBridge.deliverResult(requestId, false, null, "empty_result");
                finish();
                return;
            }
        }

        LeafFilePickerBridge.deliverResult(requestId, true, resultPaths, "");
        finish();
    }

    private void applyMimeFilter(Intent intent) {
        switch (mediaType) {
            case 1: // Image
                intent.setType("image/*");
                break;
            case 2: // Video
                intent.setType("video/*");
                break;
            case 3: // ImageOrVideo
                intent.setType("*/*");
                intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"image/*", "video/*"});
                break;
            case 0: // Any
            default:
                intent.setType("*/*");
                break;
        }
    }

    private String copyUriToLocalFile(Uri uri) {
        if (uri == null) {
            return null;
        }

        File dir = new File(getCacheDir(), "leaf/file_picker");
        if (!dir.exists() && !dir.mkdirs()) {
            return null;
        }

        String displayName = resolveDisplayName(uri);
        if (displayName == null || displayName.isEmpty()) {
            displayName = "picked_file";
        }

        String safeName = displayName.replaceAll("[\\\\/:*?\"<>|]", "_");
        String fileName = System.currentTimeMillis() + "_" + safeName;
        File outFile = new File(dir, fileName);

        ContentResolver resolver = getContentResolver();
        try (InputStream input = resolver.openInputStream(uri);
             OutputStream output = new FileOutputStream(outFile)) {
            if (input == null) {
                return null;
            }
            byte[] buffer = new byte[16 * 1024];
            int read;
            while ((read = input.read(buffer)) != -1) {
                output.write(buffer, 0, read);
            }
            output.flush();
            return outFile.getAbsolutePath();
        } catch (Throwable t) {
            if (outFile.exists()) {
                // Best-effort cleanup on partial copy failure.
                //noinspection ResultOfMethodCallIgnored
                outFile.delete();
            }
            return null;
        }
    }

    private String resolveDisplayName(Uri uri) {
        Cursor cursor = null;
        try {
            cursor = getContentResolver().query(uri, null, null, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                int index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (index >= 0) {
                    String name = cursor.getString(index);
                    if (name != null && !name.isEmpty()) {
                        return name;
                    }
                }
            }
        } catch (Throwable ignored) {
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return null;
    }
}
