package net.chentong.leaf.core;

import android.content.res.AssetManager;
import android.content.res.Resources;
import android.opengl.GLSurfaceView;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class LeafRenderer implements GLSurfaceView.Renderer {

    private AssetManager assetManager;

    public LeafRenderer(AssetManager assetManager) {
        this.assetManager = assetManager;
    }

    // --- Native 方法声明 ---
    // 加载编译好的 libsengine-core.so
    static {
        System.loadLibrary("leaf-core");
    }

    // 对应 engine_jni.cpp 中的 extern "C" 函数
    private native void nativeOnSurfaceCreated(AssetManager assetManager);
    private native void nativeOnSurfaceChanged(int width, int height, float density);
    private native void nativeOnDrawFrame();

    // --- 实现 GLSurfaceView.Renderer 接口 ---

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        // 传递 AssetManager 供 C++ 读取 main.js
        nativeOnSurfaceCreated(assetManager);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        // 获取设备像素密度 (density)，用于 NanoVG 缩放绘制
        float density = Resources.getSystem().getDisplayMetrics().density;
        nativeOnSurfaceChanged(width, height, density);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        nativeOnDrawFrame();
    }
}