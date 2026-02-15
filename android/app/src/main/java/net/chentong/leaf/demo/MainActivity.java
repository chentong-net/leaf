package net.chentong.leaf.demo;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import net.chentong.leaf.android.LeafPluginBridge;
import net.chentong.leaf.android.LeafView;

public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 直接创建并显示 Leaf 视图
        LeafView leafView = new LeafView(this);
        setContentView(leafView);
        LeafPluginBridge.bindActivity(this);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (LeafPluginBridge.onActivityResult(requestCode, resultCode, data)) {
            return;
        }
        super.onActivityResult(requestCode, resultCode, data);
    }
}
