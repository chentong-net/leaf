package net.chentong.leaf.android;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.MotionEvent;

public class LeafView extends GLSurfaceView {

    private LeafRenderer renderer;

    public LeafView(Context context) {
        super(context);
        init(context);
    }

    public LeafView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    private void init(Context context) {
        // 设置 OpenGL ES 版本为 3.0
        setEGLContextClientVersion(3);
        // 显式设置模板缓冲区
        // LFScrollView 依赖 nvgIntersectScissor 来裁剪超出视口的内容
        // NanoVG 内部利用模板缓冲区来标记画的内容
        setEGLConfigChooser(8, 8, 8, 8, 16, 8);

        // 设置渲染器
        renderer = new LeafRenderer(context.getAssets());
        setRenderer(renderer);

        // 设置渲染模式：CONTINUOUSLY 表示持续刷新（类似游戏/Flutter）
        setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        // Dispatch to renderer (will be queued and processed on GL thread)
        if (renderer != null) {
            renderer.queueTouchEvent(event);
        }
        return true;
    }
}