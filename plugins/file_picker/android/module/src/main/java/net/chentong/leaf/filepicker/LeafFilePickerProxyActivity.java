package net.chentong.leaf.filepicker;

import android.app.Activity;
import android.content.ClipData;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;

import java.util.ArrayList;
import java.util.List;

public final class LeafFilePickerProxyActivity extends Activity {

    private static final int REQUEST_FILE_PICKER = 0x4C50;

    private int requestId = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestId = LeafFilePickerBridge.readRequestId(getIntent());
        if (requestId < 0) {
            finish();
            return;
        }

        if (savedInstanceState == null) {
            openFilePicker();
        }
    }

    private void openFilePicker() {
        try {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, false);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
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

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            for (String path : paths) {
                try {
                    Uri uri = Uri.parse(path);
                    getContentResolver().takePersistableUriPermission(
                            uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
                } catch (Throwable ignored) {
                }
            }
        }

        LeafFilePickerBridge.deliverResult(requestId, true, paths, "");
        finish();
    }
}
