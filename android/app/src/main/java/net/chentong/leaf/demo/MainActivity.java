package net.chentong.leaf.demo;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import net.chentong.leaf.android.LeafView;
import net.chentong.leaf.android.PluginRegistry;
import net.chentong.leaf.android.plugin.audioplayer.AudioPlayerPlugin;
import net.chentong.leaf.android.plugin.filepicker.FilePickerPlugin;
import net.chentong.leaf.android.plugin.i18n.I18nPlugin;
import net.chentong.leaf.android.plugin.localtime.LocalTimePlugin;
import net.chentong.leaf.android.plugin.pathprovider.PathProviderPlugin;

public class MainActivity extends Activity {
    private FilePickerPlugin filePickerPlugin;
    private PathProviderPlugin pathProviderPlugin;
    private AudioPlayerPlugin audioPlayerPlugin;
    private I18nPlugin i18nPlugin;
    private LocalTimePlugin localTimePlugin;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 直接创建并显示 Leaf 视图
        LeafView leafView = new LeafView(this);
        setContentView(leafView);
        filePickerPlugin = new FilePickerPlugin(this);
        pathProviderPlugin = new PathProviderPlugin(this);
        audioPlayerPlugin = new AudioPlayerPlugin(this);
        i18nPlugin = new I18nPlugin(this);
        localTimePlugin = new LocalTimePlugin();
        PluginRegistry.getInstance().register(filePickerPlugin);
        PluginRegistry.getInstance().register(pathProviderPlugin);
        PluginRegistry.getInstance().register(audioPlayerPlugin);
        PluginRegistry.getInstance().register(i18nPlugin);
        PluginRegistry.getInstance().register(localTimePlugin);
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
