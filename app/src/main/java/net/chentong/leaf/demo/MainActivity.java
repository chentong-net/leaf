package net.chentong.leaf.demo;

import android.app.Activity;
import android.os.Bundle;
import net.chentong.leaf.core.LeafView;

public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 直接创建并显示 Leaf 视图
        LeafView leafView = new LeafView(this);
        setContentView(leafView);
    }
}