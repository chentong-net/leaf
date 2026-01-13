package net.chentong.sengine.core;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

public class SEngineView extends GLSurfaceView {

    public SEngineView(Context context) {
        super(context);
        init(context);
    }

    public SEngineView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    private void init(Context context) {
        // 设置 OpenGL ES 版本为 3.0
        setEGLContextClientVersion(3);

        // 设置渲染器
        setRenderer(new SEngineRenderer(context.getAssets()));

        // 设置渲染模式：CONTINUOUSLY 表示持续刷新（类似游戏/Flutter）
        setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
    }
}