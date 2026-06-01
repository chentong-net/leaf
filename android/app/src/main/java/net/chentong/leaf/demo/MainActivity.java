package net.chentong.leaf.demo;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import net.chentong.leaf.android.LeafView;
import net.chentong.leaf.android.PluginRegistry;
import net.chentong.leaf.android.plugin.audioplayer.AudioPlayerPlugin;
import net.chentong.leaf.android.plugin.filepicker.FilePickerPlugin;
import net.chentong.leaf.android.plugin.pathprovider.PathProviderPlugin;

public class MainActivity extends Activity {
    private FilePickerPlugin filePickerPlugin;
    private PathProviderPlugin pathProviderPlugin;
    private AudioPlayerPlugin audioPlayerPlugin;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 直接创建并显示 Leaf 视图
        LeafView leafView = new LeafView(this);
        setContentView(leafView);
        filePickerPlugin = new FilePickerPlugin(this);
        pathProviderPlugin = new PathProviderPlugin(this);
        audioPlayerPlugin = new AudioPlayerPlugin(this);
        PluginRegistry.getInstance().register(filePickerPlugin);
        PluginRegistry.getInstance().register(pathProviderPlugin);
        PluginRegistry.getInstance().register(audioPlayerPlugin);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (filePickerPlugin != null && filePickerPlugin.onActivityResult(requestCode, resultCode, data)) {
            return;
        }
    }

    @Override
    protected void onDestroy() {
        if (audioPlayerPlugin != null) {
            audioPlayerPlugin.release();
            audioPlayerPlugin = null;
        }
        super.onDestroy();
    }
}
