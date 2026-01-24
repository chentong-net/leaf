package net.chentong.leaf.android;

import android.content.res.AssetManager;
import android.content.res.Resources;
import android.opengl.GLSurfaceView;
import android.view.MotionEvent;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import java.util.LinkedList;
import java.util.Queue;

public class LeafRenderer implements GLSurfaceView.Renderer {

    private AssetManager assetManager;

    // Touch event queue (thread-safe)
    private final Queue<MotionEvent> touchEventQueue = new LinkedList<>();
    private final Object queueLock = new Object();

    public LeafRenderer(AssetManager assetManager) {
        this.assetManager = assetManager;
    }

    // --- Native 方法声明 ---
    // 加载编译好的 libleaf-android.so
    static {
        System.loadLibrary("leaf-android");
    }

    // 对应 engine_jni.cpp 中的 extern "C" 函数
    private native void nativeOnSurfaceCreated(AssetManager assetManager);
    private native void nativeOnSurfaceChanged(int width, int height, float density);
    private native void nativeOnDrawFrame();

    // New: Touch event dispatch to native
    private native void nativeDispatchTouchEvent(
        int action,
        int actionIndex,
        int pointerCount,
        int[] pointerIds,
        float[] x,
        float[] y,
        float[] pressure,
        long eventTime
    );

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
        // 1. Process touch events
        processTouchEvents();

        // 2. Render frame
        nativeOnDrawFrame();
    }

    /**
     * Queue touch event from main thread
     */
    public void queueTouchEvent(MotionEvent event) {
        // Copy event (MotionEvent will be recycled by system)
        MotionEvent eventCopy = MotionEvent.obtain(event);
        synchronized (queueLock) {
            touchEventQueue.offer(eventCopy);
        }
    }

    /**
     * Process touch events on GL thread
     */
    private void processTouchEvents() {
        while (true) {
            MotionEvent event = null;

            synchronized (queueLock) {
                if (touchEventQueue.isEmpty()) {
                    break;
                }
                event = touchEventQueue.poll();
            }

            if (event != null) {
                dispatchTouchEventToNative(event);
                event.recycle();  // Release event
            }
        }
    }

    /**
     * Convert MotionEvent to C++ format and dispatch
     */
    private void dispatchTouchEventToNative(MotionEvent event) {
        int action = event.getActionMasked();
        int actionIndex = event.getActionIndex();
        int pointerCount = event.getPointerCount();

        // Prepare data arrays
        int[] pointerIds = new int[pointerCount];
        float[] x = new float[pointerCount];
        float[] y = new float[pointerCount];
        float[] pressure = new float[pointerCount];

        for (int i = 0; i < pointerCount; i++) {
            pointerIds[i] = event.getPointerId(i);
            x[i] = event.getX(i);
            y[i] = event.getY(i);
            pressure[i] = event.getPressure(i);
        }

        long eventTime = event.getEventTime();

        // Call JNI
        nativeDispatchTouchEvent(
            action,
            actionIndex,
            pointerCount,
            pointerIds,
            x, y, pressure,
            eventTime
        );
    }
}