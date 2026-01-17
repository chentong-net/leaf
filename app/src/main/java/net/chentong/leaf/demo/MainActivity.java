package net.chentong.leaf.demo;

import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import net.chentong.leaf.core.LeafView;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // 直接创建并显示 SEngine 视图
        LeafView leafView = new LeafView(this);
        setContentView(leafView);
    }
}